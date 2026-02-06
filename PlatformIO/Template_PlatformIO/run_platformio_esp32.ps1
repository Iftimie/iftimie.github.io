# Run PlatformIO esp32 build and capture output to a log file.
# Usage: Open PowerShell in project root and run: .\run_platformio_esp32.ps1

$pio = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
$log = "./.pio_esp32_build.log"

if (!(Test-Path $pio)) {
    Write-Error "PlatformIO executable not found at $pio"
    Write-Host "If PlatformIO is installed elsewhere, edit this script to point to the correct path."
    exit 1
}

Write-Host "Running: $pio run -e esp32cam"
& $pio run -e esp32cam 2>&1 | Tee-Object -FilePath $log

Write-Host "Build finished. Log saved to $log"
if (Test-Path $log) { Get-Item $log }
