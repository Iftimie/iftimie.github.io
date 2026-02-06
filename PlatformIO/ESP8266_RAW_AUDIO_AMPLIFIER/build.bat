@echo off
REM build.bat - wrapper to prefer make, then mingw32-make, fallback to g++ compile

where make >nul 2>&1
if %errorlevel%==0 (
  echo using make
  make %*
  exit /b %errorlevel%
)

where mingw32-make >nul 2>&1
if %errorlevel%==0 (
  echo using mingw32-make
  mingw32-make %*
  exit /b %errorlevel%
)

echo make not found; compiling with g++
g++ -std=c++17 -I src -I src/interfaces src/CommandUtils.cpp src/interfaces/Globals.cpp test/host_tests.cpp test/mocks/MockEyes.cpp -o test/host_tests.exe %*
if %errorlevel% neq 0 exit /b %errorlevel%

echo Running tests...
test\host_tests.exe
exit /b %errorlevel%
