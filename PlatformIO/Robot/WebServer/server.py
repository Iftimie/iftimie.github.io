import asyncio
from typing import Set

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, Form, Request, Query
from fastapi.responses import PlainTextResponse, HTMLResponse
from pathlib import Path
import asyncio

app = FastAPI()

clients: Set[WebSocket] = set()
clients_lock = asyncio.Lock()


@app.get("/", response_class=PlainTextResponse)
def root():
    return "OK. Use WS /ws and POST /commands?cmd=blink"


@app.websocket("/ws")
async def ws_endpoint(ws: WebSocket):
    await ws.accept()
    async with clients_lock:
        clients.add(ws)
    print(f"[WS] connected: {ws.client}")

    try:
        while True:
            # You can ignore incoming messages or log them
            msg = await ws.receive_text()
            print(f"[WS] rx from {ws.client}: {msg}")
    except WebSocketDisconnect:
        pass
    finally:
        async with clients_lock:
            clients.discard(ws)
        print(f"[WS] disconnected: {ws.client}")


@app.post("/commands")
async def broadcast_commands(request: Request, cmd: str = Query(None)):
    """Broadcast a command string to all connected WS clients.

    Accepts the command as either a query parameter `cmd=` (string or
    URL-encoded JSON) or as a JSON body (application/json) containing the
    grouped array-of-arrays structure. The server treats the received value
    opaquely and forwards it to all connected WebSocket clients.
    """
    # Prefer JSON body if present (serialize it back to a string), otherwise
    # fall back to the `cmd` query parameter.
    body_cmd = None
    try:
        raw = await request.body()
        if raw:
            import json
            try:
                parsed = json.loads(raw.decode('utf-8'))
            except Exception:
                parsed = None
            if isinstance(parsed, (list, dict)):
                body_cmd = json.dumps(parsed)
    except Exception:
        body_cmd = None

    final_cmd = cmd if (cmd is not None and cmd != "") else body_cmd
    if final_cmd is None:
        return PlainTextResponse("Missing cmd (provide ?cmd=... or JSON body)", status_code=400)
    dead = []
    async with clients_lock:
        targets = list(clients)

    for ws in targets:
        try:
            print("will send to", ws.client, ":", final_cmd)
            await ws.send_text(cmd)
        except Exception:
            dead.append(ws)

    if dead:
        async with clients_lock:
            for ws in dead:
                clients.discard(ws)

    return {"sent": final_cmd, "clients": len(targets), "removed_dead": len(dead)}


@app.get("/ui", response_class=HTMLResponse)
async def ui():
    """Serve simple web UI for sending commands and viewing WS messages."""
    p = Path(__file__).with_name("web_ui.html")
    if p.exists():
        return HTMLResponse(p.read_text())
    return HTMLResponse("<html><body><h3>UI not found</h3></body></html>")


@app.get("/ui_v2", response_class=HTMLResponse)
async def ui_v2():
    """Serve the v2 clip-editor UI if present."""
    p = Path(__file__).with_name("web_ui_v2.html")
    if p.exists():
        return HTMLResponse(p.read_text())
    return HTMLResponse("<html><body><h3>UI v2 not found</h3></body></html>")


@app.post("/save_snippet")
async def save_snippet(line: str = Form(...)):
    """Append a single line (e.g. a curl command) to `personality.txt`.

    This uses a thread to avoid blocking the event loop for file I/O.
    """
    p = Path(__file__).with_name("personality.txt")

    def _append():
        with p.open("a", encoding="utf-8") as f:
            f.write(line.rstrip("\n") + "\n")

    await asyncio.to_thread(_append)
    return {"saved": line}
