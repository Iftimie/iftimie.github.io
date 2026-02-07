# PIC10F200 PWM

Software PWM on PIC10F200 with a duty-cycle ramp for LED brightness control.

## What it does
- Implements software PWM on GP1 using a `counter < duty` comparison.
- Ramps duty up and down between 0 and 30 for a smooth brightness fade.
- Sets PWM frequency with a short delay loop.

## Hardware
- PIC10F200
- LED (or small load on GP1)

## Software / Tools
- MPLAB / assembler

## How to run
- Build in MPLAB/MPASM and run on a PIC10F200.
- Tweak `DELAY` and the duty bounds to adjust frequency and fade speed.

## Media
- Images: TODO
- Scope capture: TODO

## Notes
- Compared to `PIC10F200_LED_OSCILLATE`, this version actively changes duty to create a fade.
- Uses a single pin PWM signal rather than polarity switching with dead-time.
