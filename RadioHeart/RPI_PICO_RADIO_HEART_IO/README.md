# Raspberry Pi Pico Radio Heart (PlatformIO)

This refactors the original Arduino `.ino` into a PlatformIO project with modular sources for radio, touch I/O, LEDs, and protocol.

## Prerequisites
- PlatformIO Core installed (`pip install platformio`) or VS Code with PlatformIO extension.

## Build & Upload

```bash
pio run -e pico
pio run -e pico -t upload
pio device monitor -e pico -b 115200
```

## Configuration
- Set device macros via `platformio.ini` or CLI:
  - `MY_ID` (default 1)
  - `PEER_ID` (default 2)
  - `TOUCH_ACTIVE_HIGH` (default 1)
- Adjust pins/timing in `include/pins.h` and `include/config.h`.

## Project Structure
- `src/main.cpp`: Orchestrates setup/loop, calls modules.
- `src/radio.cpp` + `include/radio.h`: LLCC68 + SPI init, TX/RX, RSSI/SNR.
- `src/touch_io.cpp` + `include/touch_io.h`: Touch read + blocking record.
- `src/leds.cpp` + `include/leds.h`: Signal fade, builtin level, online LED playback.
- `src/protocol.cpp` + `include/protocol.h`: Packet structs & quality mapping.
- `src/utils.cpp` + `include/utils.h`: Timing helper.
- `include/pins.h`, `include/config.h`: Hardware pins and constants.

## Notes
- Optimization: `-O2` and LTO enabled.
- Upload protocol: `picotool` (use BOOTSEL to flash if needed).
- Dependencies: RadioLib (`jgromes/RadioLib`).
