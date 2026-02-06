# Copilot / AI Agent Instructions — WebServer

Summary
- Small FastAPI-based WebSocket broadcaster. Primary entry: `server.py` runs a single-process ASGI app that accepts WebSocket clients at `/ws` and broadcasts opaque command strings sent via POST `/commands?cmd=...`.

Quick start
- Run locally: `uvicorn server:app --host 0.0.0.0 --port 8765`
- Test sending a command (examples live in `personality.txt`):
  - `curl -X POST "http://127.0.0.1:8765/commands?cmd=up"`

Big picture
- `server.py`: single-file FastAPI app. Maintains `clients: Set[WebSocket]` protected by `clients_lock` (an `asyncio.Lock`) and implements simple broadcast semantics.
- Clients connect over WebSocket to `/ws` and receive raw command strings. The server does not parse command syntax — it only forwards strings.
- `personality.txt` contains curated `curl` examples and the de-facto command formats you must preserve when changing behavior (e.g. `eyes_seq:`, `audio:`, `move:`, `text:`).
- `firewall_settings` documents Windows/WSL portproxy and firewall rules used to expose port `8765` from WSL to the host. Changes that alter the port or binding will require updating this file or the host firewall rules.

Key patterns and conventions for edits
- Keep endpoints async and non-blocking. Long-running or CPU-heavy work must be moved to background tasks or external services to avoid blocking the event loop.
- Preserve the global `clients` set + `clients_lock` pattern for connection management. When broadcasting, follow the current pattern of collecting `targets = list(clients)` under lock, then send outside the lock and discard dead clients afterward.
- The server treats commands as opaque strings. If you introduce parsing or richer command handling, maintain backward compatibility with existing sequences in `personality.txt`.
- Avoid adding blocking stdio or heavy synchronous I/O in request handlers. Use `asyncio` primitives or FastAPI BackgroundTasks.

Developer workflows and testing
- Run server: `uvicorn server:app --host 0.0.0.0 --port 8765`.
- Manual test: use `curl` (examples in `personality.txt`) or a WebSocket client such as `websocat` to open `ws://localhost:8765/ws` and observe broadcasts.
- No unit tests are present; prefer minimal integration/manual tests after changes. If adding tests, use `pytest` and an ASGI test client (e.g., `httpx.AsyncClient` or `starlette.testclient`).

Integration and external dependencies
- External clients (robot controllers, browsers, or embedded devices) connect over WebSocket and expect opaque strings exactly as in `personality.txt`.
- The project assumes port `8765` is reachable; on Windows + WSL the `firewall_settings` file shows `netsh`/`New-NetFirewallRule` commands required to forward/expose the port from WSL to the host IP.

When making changes, include these explicit checks
- Verify `uvicorn` still serves `server:app` on the same port (or update `firewall_settings` if you change it).
- Run several concurrent WebSocket clients locally and ensure broadcasting still reaches all clients and dead clients are removed.
- If you introduce command parsing, add examples to `personality.txt` and keep the original broadcast-only fallback.

Relevant files
- [.github/copilot-instructions.md](.github/copilot-instructions.md)
- [server.py](server.py#L1-L200)
- [personality.txt](personality.txt#L1-L200)
- [firewall_settings](firewall_settings#L1-L200)

If anything in this summary is unclear or you want me to expand a section (e.g., add example tests or a small client script), tell me which part and I'll iterate.
