```instructions
# Copilot instructions — ESP32 CAM Robot Eyes

Concise guidance to help an AI coding agent be productive in this repository.

- **Big picture**: Firmware for an ESP32-CAM that runs multiple FreeRTOS tasks to present "robot eyes":
  - Network I/O (WebSocket) → command parsing (`src/main.cpp`, `tasks/WebSocketTask.*`).
  - Eyes subsystem (`src/MD_RobotEyes*.{h,cpp}` + `tasks/EyesTask.*`) drives an LED matrix via MD_MAX72XX.
  - Audio (`tasks/AudioTask.*`) drives the buzzer using ESP32 LEDC tones.
  - Motors (`tasks/MotorsTask.*`) forward commands over `Serial2` to an ATtiny.

- **Core files to read first**:
  - [src/main.cpp](src/main.cpp) — app entrypoint, global queues, `onWsEvent` handler.
  - [src/MD_RobotEyes.h](src/MD_RobotEyes.h) / [src/MD_RobotEyes.cpp](src/MD_RobotEyes.cpp) — eye API and animations.
  - [src/MD_RobotEyes_Data.h](src/MD_RobotEyes_Data.h) — static fonts/frames used by animations.
  - [tasks/](tasks/) — task implementations: `EyesTask`, `AudioTask`, `MotorsTask`, `WebSocketTask`.
  - [include/CommandUtils.*](include/) & [tasks/TaskCommon.h](tasks/TaskCommon.h) — parsing helpers and shared constants.

- **Runtime architecture & dataflow**:
  - WebSocket text messages are parsed in `onWsEvent` and dispatched to three FreeRTOS queues: `g_cmd_q` (eyes), `g_audio_q`, `g_motor_q`.
  - Each queue has a dedicated consumer task which parses its payload and calls hardware adapters (`MD_RobotEyesAdapter`, buzzer, UART forward).
  - `MD_RobotEyes` exposes `begin()`, `setAnimation()`, `setText()`, `runAnimation()` — `runAnimation()` must be polled frequently inside `taskEyes`.

- **Message / command patterns (use exact formats)**:
  - `eyes:angry` or simply `angry` — single emotion
  - `text:HELLO` — scrolling text on eyes
  - `eyes_seq:up,200;blink,200;text:HI,600;right,200` — sequence steps (see `parseEyesSequence` in `src/main.cpp`)
  - `eyes_seq:stop` — stop sequence
  - `audio:C4,200;REST,50;E4,200` — note,ms pairs; `audio:stop` stops
  - `move:...` — forwarded to `Serial2` (ATtiny)

  - **Grouped JSON commands (array-of-array)**:
    - Format: JSON outer array of inner arrays of strings, e.g. `[ ["eyes:angry","audio:C4,200"], ["move:FWD,500"] ]`.
    - Semantics: each inner array is executed in parallel; the outer array is executed sequentially (the device waits for all commands in one inner group to finish before starting the next group).
    - Implementation: parsed and executed in [src/tasks/WebSocketTask.cpp](src/tasks/WebSocketTask.cpp) via `processGroupedJson()` which assigns IDs to enqueued commands and waits for `DoneEvent`s on `g_done_q`.
    - Completion: tasks send `DoneEvent` when they finish their work (see `tasks/AudioTask.cpp`, `tasks/EyesTask.cpp`, `tasks/MotorsTask.cpp`). The WS task stashes early `DoneEvent`s and ignores non-string/invalid items.
    - Host helpers: `parseGroupedJson()` in [src/CommandUtils.cpp](src/CommandUtils.cpp) is a lenient parser used by host tests and can be used to validate messages before sending.
    - Example (first runs eyes+audio, then motors after both complete):
      - `[ ["eyes:angry","audio:C4,200"], ["move:FWD,500"] ]`


- **Hardware pins & constants (change with caution)**:
  - LED matrix: `DATA_PIN = 13`, `CLK_PIN = 14`, `CS_PIN = 15`, `MAX_DEVICES = 2` (see [src/main.cpp](src/main.cpp)).
  - Motor TX: `MOTOR_TX_PIN = 12`, UART `Serial2` @ 9600.
  - Buzzer: `BUZZER_PIN = 4` using LEDC (`ledcAttachPin`, `ledcWriteTone`).
  - Keep pin changes minimal and verify physically before committing.

- **Build / test / debug workflows**:
  - Build (esp32): `platformio run --environment esp32cam`
  - Upload (esp32): `platformio run --environment esp32cam --target upload`
  - Serial monitor: `platformio device monitor --environment esp32cam` (115200)
  - Native/host unit tests: `platformio test -e native` (or run `run_platformio_native.ps1` on Windows).
  - Convenience scripts available: `build.bat`, `build.ps1`, `run_platformio_esp32.ps1`, `run_platformio_native.ps1`.

- **Project-specific conventions**:
  - FreeRTOS-first: tasks are created with `xTaskCreatePinnedToCore`. Favor small stacks and pinned tasks.
  - No dynamic allocations in real-time paths: use static buffers, fixed-size char arrays and `CMD_MAX_LEN` (160).
  - Parsing helpers (`skipSpaces`, `startsWithNoCase`, `afterPrefix`) live in `include/CommandUtils.*`; follow their pattern.
  - Hardware access goes through adapters in `interfaces/` (e.g., `MD_RobotEyesAdapter`) to make unit-testing easier.

- **Search hints / where to change things**:
  - Command parsing & top-level dispatch: [src/main.cpp](src/main.cpp)
  - Eye animations & font data: [src/MD_RobotEyes*.cpp](src/MD_RobotEyes.cpp) and [src/MD_RobotEyes_Data.h](src/MD_RobotEyes_Data.h)
  - Task implementations: [tasks/](tasks/)
  - Mocks and host tests: [test/](test/) (see `host_tests.cpp`, `mocks/`)

- **Risky changes — avoid without human sign-off**:
  - Modifying `platformio.ini` build flags (PSRAM/flash options).
  - Changing pins or core task timing that affects `runAnimation()` frequency.

If you want I can also generate a small host-side WebSocket test client, add unit-test skeletons, or run the native tests—tell me which.
``` # Copilot instructions — ESP32 CAM Robot Eyes

Quick, focused guidance to help an AI coding agent be immediately productive in this repository.

- **Big picture**: This is an Arduino/PlatformIO project for an ESP32-based robot face (ESP32-CAM board). The firmware runs multiple FreeRTOS tasks to control:
  - WebSocket connectivity and message parsing (`taskWebSocket` in `src/main.cpp`).
  - LED-matrix eyes and animations (`taskEyes`, using `MD_RobotEyes` + `MD_MAX72XX`).
  - Buzzer audio sequences (`taskAudio`).
  - Motor UART forwarding to an ATtiny (`taskMotors`).

- **Core files to read first**:
  - `src/main.cpp` — entrypoint, message parsing, task creation, and queue-based IPC.
  - `src/MD_RobotEyes.h` / `src/MD_RobotEyes.cpp` — eye animation API and implementation.
  - `src/MD_RobotEyes_Data.h` — static font/sequence data used by animations.
  - `platformio.ini` — build environment `esp32cam` and target-specific flags (PSRAM disabled, safe flash settings).

- **Runtime architecture & dataflow (short)**:
  - WebSocket event handler `onWsEvent` receives text messages and enqueues them on one of three FreeRTOS queues: `g_cmd_q` (eyes), `g_audio_q` (audio), `g_motor_q` (motors).
  - Each queue is consumed by a dedicated task (`taskEyes`, `taskAudio`, `taskMotors`) which implements parsing and hardware actions.
  - `taskEyes` uses `MD_RobotEyes` methods: `begin()`, `setAnimation()`, `setText()`, `runAnimation()`; `runAnimation()` should be called frequently from the task loop.

- **Message command patterns (copy these examples exactly)**:
  - Eyes single emotion: `eyes:angry` or just `angry`
  - Looping text on eyes: `text:HELLO`
  - Eyes sequence: `eyes_seq:up,200;blink,200;text:HI,600;right,200` (see `parseEyesSequence` and `parseEyeStep` in `src/main.cpp`)
  - Stop sequence: `eyes_seq:stop`
  - Audio sequence: `audio:C4,200;REST,50;E4,200` (format `NOTE,ms;NOTE,ms;...`); stop with `audio:stop`
  - Motor forward: `move:...` → forwarded over UART (`Serial2`) to the ATtiny

- **Hardware / pins & constants (refer when coding)**:
  - LED matrix: `DATA_PIN = 13`, `CLK_PIN = 14`, `CS_PIN = 15`, `MAX_DEVICES = 2` (see top of `src/main.cpp`).
  - Motor TX pin: `MOTOR_TX_PIN = 12`, baud `9600`.
  - Buzzer pin: `BUZZER_PIN = 4`, new LEDC API used (`ledcAttach`, `ledcWriteTone`).

- **Build / run / debug workflows**:
  - Build and upload using PlatformIO (environment `esp32cam`):
    - `platformio run --environment esp32cam` (build)
    - `platformio run --environment esp32cam --target upload` (upload)
    - Use the VS Code PlatformIO tasks shown in the project for convenience.
  - Serial monitor: `platformio device monitor --environment esp32cam` (115200 configured in `platformio.ini`).
  - Wi-Fi + WebSocket: device connects to `WIFI_SSID`/`WIFI_PASS` from `src/main.cpp`. To test interactions, run a WebSocket server at `WS_HOST:WS_PORT` and send commands to path `/ws`.

- **Project-specific conventions & patterns**:
  - FreeRTOS-first: The firmware uses pinned tasks (`xTaskCreatePinnedToCore`) and static buffers (avoid heap allocations inside tasks). Follow the same pattern for new tasks: create a queue, use static buffers, and keep stack sizes small but sufficient.
  - Parsing style: string commands are parsed with small helper functions (`skipSpaces`, `startsWithNoCase`, `afterPrefix`) and stored in fixed-size static buffers. Preserve this pattern for memory safety.
  - Minimal dynamic memory: prefer static buffers, copy into provided buffers, avoid std::string or heap allocations in real-time tasks.

- **Search hints for quick navigation**:
  - Search for `eyes_seq:`, `audio:`, `move:` to find parsing/handling logic.
  - `MD_RobotEyes` class shows how animations and text are queued and displayed; use it as the canonical API for eye changes.

- **What an AI agent should not change without human approval**:
  - PlatformIO board/build flags in `platformio.ini` (PSRAM and flash mode tweaks are important for this board).
  - Pin assignments without hardware verification.
  - Blocking long-running calls inside tasks that expect frequent `runAnimation()` calls — this breaks animation timing.

- **When adding features**:
  - Follow existing queue/task pattern: add a queue, a task consumer, and push from `onWsEvent` or existing tasks.
  - Use static buffers for command parsing and keep max command length `CMD_MAX_LEN` (160).

If anything here is unclear or you want more examples (unit tests, simulated WS client, or sample host server scripts), tell me which area to expand.
