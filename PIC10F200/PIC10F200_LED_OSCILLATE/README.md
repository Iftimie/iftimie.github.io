# PIC10F200 LED Oscillate

PWM-style LED driver that holds a fixed brightness by toggling GPIOs with timed delays.

## What it does
- Drives GP0/GP1 as polarity pins and GP2 as enable (L293D-style), alternating direction with a short dead-time.
- Uses a nested delay loop to set the switching rate, keeping the LED at a steady brightness.

## Hardware
- PIC10F200
- LED or small load via L293D (IN1/IN2/EN1)

## Software / Tools
- MPLAB / assembler

## How to run
- Build in MPLAB/MPASM and run on a PIC10F200.
- Adjust `DELAY_2P5MS` constants to change the PWM frequency.

## Media
- Images: TODO

## Notes
- This is a fixed-duty driver, not a ramping brightness effect.
