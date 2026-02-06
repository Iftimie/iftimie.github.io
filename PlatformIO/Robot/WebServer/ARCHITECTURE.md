Overview

This project is a tiny local FastAPI-based broadcaster that accepts command strings (or grouped JSON) via HTTP and forwards them to connected WebSocket clients. The primary components are:

- `server.py` — FastAPI app. Endpoints:
  - `GET /` — health text
  - `GET /ui` — serves the `web_ui.html` UI
  - `GET /ws` — WebSocket endpoint; clients connect and receive opaque strings
  - `POST /commands` — broadcast commands to connected WS clients. Accepts either:
    - `?cmd=<string>` query parameter (legacy behavior), or
    - JSON body (application/json) containing the grouped array-of-arrays format.

- `web_ui.html` — browser UI for composing commands. It builds a grouped JSON array-of-arrays where:
  - The outer array is executed sequentially on the device.
  - Each inner array contains strings representing actions that run in parallel.
  - Example grouped JSON: `[["audio:C4,500","move:FWD,500"],["eyes:angry","text:Hello","move:LEFT,250"]]`

- ESP32 robot code (outside this repo) — expects grouped JSON (array-of-arrays) and implements `processGroupedJson()` in `src/tasks/WebSocketTask.cpp`. Each inner array is executed in parallel; the outer array is sequential. Tasks post `DoneEvent`s to signal completion.

Grouped JSON semantics

- Format: JSON outer array of inner arrays of strings. Each inner array is a parallel group.
- Strings are opaque tokens interpreted by device tasks. Common prefixes used by UI and examples:
  - `eyes:NAME` or `eyes_seq:...` — eye animations or sequences; many animations have hardcoded durations in `MD_RobotEyes_Data.h`.
  - `text:...` — display text on eyes (treated as separate token).
  - `audio:NOTE,DURATION[;NOTE,DURATION;...]` — audio sequences; durations provided by UI.
  - `move:DIR,AMOUNT` — motion tokens; durations or distances provided by UI.
  - `pause:MS` — optional helper to introduce delays without other actions.

Server behavior and compatibility

- Backward-compatible: `POST /commands?cmd=<string>` is still supported and the server will forward the string unchanged.
- New behavior: `POST /commands` with a JSON body (application/json) will be accepted; the server serializes the JSON body back to a compact string and broadcasts it.
- The server intentionally treats commands opaquely — parsing/validation is performed by the robot device. This preserves compatibility with existing devices.

UI changes

- The UI now builds grouped JSON (array-of-arrays) and sends a single `POST /commands?cmd=<urlencoded-json>` request.
- The UI also supports saving the generated curl line to `personality.txt` for later use.

What to change on the robot (ESP32) side if needed

- No immediate change required if the robot already implements `processGroupedJson()` and accepts grouped JSON.
- If the robot only supports single command strings, either:
  - Keep using the legacy `?cmd=` string format (UI still supports it), or
  - Implement grouped JSON parsing via `parseGroupedJson()` (the host helper in `src/CommandUtils.cpp` is useful)

Developer notes

- To test grouped JSON via curl (examples):

  curl -X POST http://127.0.0.1:8765/commands -H "Content-Type: application/json" \
    -d '[ ["audio:C4,500","move:FWD,500"], ["eyes:angry","text:Hi"] ]'

- The server's `POST /commands` returns JSON describing the broadcasted value and number of clients.

- Keep the server non-blocking; heavy parsing or validation should be moved to background tasks if necessary.

Questions & next steps

- Do you want the server to perform validation of the grouped JSON before forwarding (lenient checks), or keep the server fully opaque and let the device handle validation?
- I can add an example client script for local testing that connects over WebSocket and logs incoming commands.
