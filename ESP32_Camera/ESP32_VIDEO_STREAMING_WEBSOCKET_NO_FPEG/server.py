import asyncio
import time
from typing import Optional, Set
import io
import struct
from PIL import Image

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException
from fastapi.responses import Response, StreamingResponse

app = FastAPI()

UPLOAD_TOKEN = "change-me"

latest_jpeg: Optional[bytes] = None
latest_ts: float = 0.0

frame_lock = asyncio.Lock()
frame_event = asyncio.Event()

# Optional: WS viewers that want frames too
viewers: Set[WebSocket] = set()
viewers_lock = asyncio.Lock()


@app.get("/health")
def health():
    return {"ok": True, "has_frame": latest_jpeg is not None, "ts": latest_ts}


@app.get("/latest.jpg")
async def latest():
    async with frame_lock:
        if latest_jpeg is None:
            raise HTTPException(status_code=404, detail="no frame yet")
        data = latest_jpeg
        ts = latest_ts

    return Response(
        content=data,
        media_type="image/jpeg",
        headers={"Cache-Control": "no-store", "X-Frame-Timestamp": str(ts)},
    )


@app.get("/stream.mjpeg")
async def stream():
    boundary = "frame"

    async def gen():
        last_sent = 0.0
        while True:
            try:
                await asyncio.wait_for(frame_event.wait(), timeout=10.0)
            except asyncio.TimeoutError:
                continue

            async with frame_lock:
                if latest_jpeg is None:
                    continue
                ts = latest_ts
                jpg = latest_jpeg

            if ts == last_sent:
                continue
            last_sent = ts

            yield (
                f"--{boundary}\r\n"
                "Content-Type: image/jpeg\r\n"
                f"Content-Length: {len(jpg)}\r\n"
                "\r\n"
            ).encode("utf-8") + jpg + b"\r\n"

    return StreamingResponse(
        gen(),
        media_type=f"multipart/x-mixed-replace; boundary={boundary}",
        headers={"Cache-Control": "no-store"},
    )

@app.websocket("/ws/upload")
async def ws_upload(ws: WebSocket):
    token = ws.query_params.get("token")
    if token != UPLOAD_TOKEN:
        await ws.close(code=1008)
        return

    await ws.accept()
    print("[WS-UPLOAD] connected")

    last_log = 0.0
    frames = 0

    try:
        while True:
            msg = await ws.receive()

            if "bytes" not in msg or msg["bytes"] is None:
                continue

            raw_msg = msg["bytes"]
            now = time.time()

            if len(raw_msg) < 8:
                print(f"[WS-UPLOAD] bad packet too small: {len(raw_msg)}")
                continue

            w, h = struct.unpack("<II", raw_msg[:8])
            raw = raw_msg[8:]

            if w <= 0 or h <= 0 or w > 2000 or h > 2000:
                print(f"[WS-UPLOAD] bad dims: {w}x{h}")
                continue

            expected = w * h
            if len(raw) != expected:
                print(f"[WS-UPLOAD] size mismatch got={len(raw)} expected={expected} ({w}x{h})")
                continue

            img = Image.frombytes("L", (w, h), raw)
            buf = io.BytesIO()
            img.save(buf, format="JPEG", quality=75, optimize=True)
            jpg = buf.getvalue()

            async with frame_lock:
                global latest_jpeg, latest_ts
                latest_jpeg = jpg
                latest_ts = now
                frame_event.set()
                frame_event.clear()

            async with viewers_lock:
                dead = []
                for v in viewers:
                    try:
                        await v.send_bytes(jpg)
                    except Exception:
                        dead.append(v)
                for v in dead:
                    viewers.discard(v)

            frames += 1
            if now - last_log >= 1.0:
                print(f"[WS-UPLOAD] ~FPS={frames} dims={w}x{h} raw={len(raw)} jpeg={len(jpg)}")
                frames = 0
                last_log = now

    except WebSocketDisconnect:
        print("[WS-UPLOAD] disconnected")
    except Exception as e:
        print("[WS-UPLOAD] error:", e)
        try:
            await ws.close()
        except Exception:
            pass



@app.websocket("/ws/view")
async def ws_view(ws: WebSocket):
    await ws.accept()
    async with viewers_lock:
        viewers.add(ws)
    print("[WS-VIEW] connected")

    try:
        while True:
            await asyncio.sleep(60)
    finally:
        async with viewers_lock:
            viewers.discard(ws)
        print("[WS-VIEW] disconnected")

