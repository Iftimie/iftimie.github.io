@echo off
echo === ESP32-CAM Flashing Troubleshooter ===
echo.
echo Step 1: Check current COM ports
powershell -Command "[System.IO.Ports.SerialPort]::getportnames() | ForEach-Object { $_ }"
echo.
echo Step 2: Check if device is responding
echo Make sure:
echo   - ESP32-CAM is powered OFF right now
echo   - USB cable is securely connected
echo   - No other programs are using COM port
echo.
echo Step 3: Proper Flashing Sequence
echo   1. Connect GPIO 0 to GND (IMPORTANT!)
echo   2. Connect 5V power (if available)
echo   3. Connect USB to computer
echo   4. Wait 2 seconds
echo   5. Start upload IMMEDIATELY
echo.
echo Step 4: Alternative - Try esptool directly
echo   Command: esptool.py --port [COM_PORT] --baud 115200 chip_id
echo.
echo Step 5: If still failing, try Arduino IDE
echo   Sometimes Arduino IDE handles ESP32-CAM better
echo.
pause