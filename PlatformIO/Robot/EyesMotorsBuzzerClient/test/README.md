# Host tests

Build and run the lightweight host test harness (requires a C++17 compiler).

PowerShell example (from project root):

```powershell
g++ -std=c++17 -I src -I src/interfaces src/CommandUtils.cpp src/interfaces/Globals.cpp test/host_tests.cpp test/mocks/MockEyes.cpp -o test/host_tests.exe
.
\test\host_tests.exe
```

Note: The build compiles a small fallback JSON parser if `ArduinoJson.h` is not available on the host.
