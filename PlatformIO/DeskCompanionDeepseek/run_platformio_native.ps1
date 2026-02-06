# Run PlatformIO native build and capture output to a log file.
# Usage: Open PowerShell in project root and run: .\run_platformio_native.ps1

$pio = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
$log = "./.pio_native_build.log"

if (!(Test-Path $pio)) {
    Write-Error "PlatformIO executable not found at $pio"
    Write-Host "If PlatformIO is installed elsewhere, edit this script to point to the correct path."
    exit 1
}

Write-Host "Running: $pio run -e native"
& $pio run -e native 2>&1 | Tee-Object -FilePath $log

Write-Host "Build finished. Log saved to $log"
if (Test-Path $log) { Get-Item $log }
