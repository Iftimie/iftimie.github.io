# build.ps1 - wrapper to prefer make, then mingw32-make, fallback to g++ compile
param(
    [Parameter(ValueFromRemainingArguments=$true)]
    $Args
)

if (Get-Command make -ErrorAction SilentlyContinue) {
    Write-Host "using make"
    & make @Args
    exit $LASTEXITCODE
} elseif (Get-Command mingw32-make -ErrorAction SilentlyContinue) {
    Write-Host "using mingw32-make"
    & mingw32-make @Args
    exit $LASTEXITCODE
} else {
    Write-Host "make not found; compiling with g++"
    & g++ -std=c++17 -I src -I src/interfaces src/CommandUtils.cpp src/interfaces/Globals.cpp test/host_tests.cpp test/mocks/MockEyes.cpp -o test/host_tests.exe @Args
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host "Running tests..."
    & .\test\host_tests.exe
    exit $LASTEXITCODE
}
