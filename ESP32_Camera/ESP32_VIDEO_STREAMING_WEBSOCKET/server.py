import asyncio
import time
from typing import Optional, Set

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
    # token via query param: /ws/upload?token=...
    token = ws.query_params.get("token")
    if token != UPLOAD_TOKEN:
        await ws.close(code=1008)  # policy violation
        return

    await ws.accept()
    print("[WS-UPLOAD] connected")

    last_log = 0.0
    frames = 0

    try:
        while True:
            msg = await ws.receive()

            if "bytes" in msg and msg["bytes"] is not None:
                jpg = msg["bytes"]
                now = time.time()

                async with frame_lock:
                    global latest_jpeg, latest_ts
                    latest_jpeg = jpg
                    latest_ts = now
                    frame_event.set()
                    frame_event.clear()

                # Optional: broadcast to WS viewers (if any)
                async with viewers_lock:
                    dead = []
                    for v in viewers:
                        try:
                            await v.send_bytes(jpg)
                        except Exception:
                            dead.append(v)
                    for v in dead:
                        viewers.discard(v)

                # lightweight FPS log once per second
                frames += 1
                if now - last_log >= 1.0:
                    print(f"[WS-UPLOAD] ~FPS={frames}")
                    frames = 0
                    last_log = now

            elif "text" in msg and msg["text"] is not None:
                # ignore or handle control messages
                pass

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
            # keep connection open; ignore messages
            _ = await ws.receive_text()
    except Exception:
        pass
    finally:
        async with viewers_lock:
            viewers.discard(ws)
        print("[WS-VIEW] disconnected")
