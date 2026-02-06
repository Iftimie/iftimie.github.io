import asyncio
import time
from typing import Optional

from fastapi import FastAPI, Request, Response, HTTPException
from fastapi.responses import StreamingResponse

app = FastAPI()

# ---- simple auth ----
UPLOAD_TOKEN = "change-me"  # put this in env later

# ---- in-memory latest frame ----
latest_jpeg: Optional[bytes] = None
latest_ts: float = 0.0

# For streaming: event notifies viewers a new frame arrived
frame_event = asyncio.Event()
frame_lock = asyncio.Lock()


@app.get("/health")
def health():
    return {"ok": True, "has_frame": latest_jpeg is not None, "ts": latest_ts}


@app.post("/upload")
async def upload(request: Request):
    token = request.headers.get("x-token")
    if token != UPLOAD_TOKEN:
        raise HTTPException(status_code=401, detail="bad token")

    ctype = request.headers.get("content-type", "")
    if "image/jpeg" not in ctype:
        raise HTTPException(status_code=415, detail="expected image/jpeg")

    body = await request.body()
    if not body or len(body) < 100:
        raise HTTPException(status_code=400, detail="empty/too small")

    global latest_jpeg, latest_ts
    async with frame_lock:
        latest_jpeg = body
        latest_ts = time.time()
        frame_event.set()
        frame_event.clear()

    return {"ok": True, "bytes": len(body), "ts": latest_ts}


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
        headers={
            "Cache-Control": "no-store, no-cache, must-revalidate, max-age=0",
            "Pragma": "no-cache",
            "X-Frame-Timestamp": str(ts),
        },
    )


@app.get("/stream.mjpeg")
async def stream():
    boundary = "frame"

    async def gen():
        last_sent = 0.0
        while True:
            # wait for a new frame (or timeout to keep connection alive)
            try:
                await asyncio.wait_for(frame_event.wait(), timeout=10.0)
            except asyncio.TimeoutError:
                # keep-alive: send nothing, just continue
                pass

            async with frame_lock:
                if latest_jpeg is None:
                    continue
                ts = latest_ts
                jpg = latest_jpeg

            # avoid spamming duplicate frames if no new ones arrived
            if ts == last_sent:
                continue
            last_sent = ts

            chunk = (
                f"--{boundary}\r\n"
                "Content-Type: image/jpeg\r\n"
                f"Content-Length: {len(jpg)}\r\n"
                "\r\n"
            ).encode("utf-8") + jpg + b"\r\n"

            yield chunk

    return StreamingResponse(
        gen(),
        media_type=f"multipart/x-mixed-replace; boundary={boundary}",
        headers={"Cache-Control": "no-store"},
    )
