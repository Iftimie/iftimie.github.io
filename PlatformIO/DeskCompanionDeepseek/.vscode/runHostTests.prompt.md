---
name: runHostTests
description: Compile and run host unit tests for a C/C++ embedded project.
argument-hint: projectRoot, sourceFiles, testFiles, includeDirs, optionalBuildTools
---
You are given a C/C++ embedded project and asked to compile and run host/unit tests on the host machine. Follow these steps and produce reproducible commands, fixes, and a small build helper:

1. Inspect: list the project root layout and locate `sourceFiles`, `testFiles`, and `includeDirs`. Identify any test source that directly `#include`s a `.cpp` implementation file instead of a header.

2. Compile command: produce a single `g++` command that compiles the tests and required sources with `-std=c++17` (or the requested standard), the provided include directories, and outputs an executable named `test/host_tests(.exe)`.
   - Example pattern: `g++ -std=c++17 -I {includeDirs} {sourceFiles} {testFiles} -o {output}`

3. Detect and fix duplicate-definition issues: if linking errors arise from duplicate symbols because a test `#include`s `*.cpp`, propose two safe fixes and implement one:
   - Preferred fix: change the test to `#include` the corresponding header and compile the implementation `.cpp` once.
   - Alternate fix: if test intentionally includes an implementation, exclude that `.cpp` from the compile list.
   Provide the exact minimal code edit (diff) or a concise instruction to apply.

4. Makefile: generate a small cross-platform `Makefile` with targets `all`/`build`, `run`, and `clean` that:
   - Uses `g++` and the same flags as in step 2.
   - Produces `test/host_tests` (and `test/host_tests.exe` on Windows).
   - Keeps rules minimal and uses variables for compiler and flags.

5. Windows wrappers: produce `build.bat` and `build.ps1` that:
   - Prefer `make`, then `mingw32-make`, and fall back to invoking the `g++` command.
   - Support `clean` and `run` semantics.
   - Exit with the underlying command's exit code.

6. Run and report: run the build (or show the exact commands to run locally), capture stdout/stderr, and summarize:
   - Success: print "ALL TESTS PASSED" or equivalent output and exit code 0.
   - Failure: paste the relevant compiler/linker error lines and explain root cause.

7. Next steps: suggest one or two small follow-ups (e.g., patch test to include headers, add CI job, or add `make` detection improvements).

Output format:
- Step summary (1–2 lines).
- Exact shell commands to run (copy-paste).
- Any file edits or added files with full content (Makefile, build.bat, build.ps1, minimal diffs).
- Short explanation of why a duplicate-definition occurred and which fix you applied.

Use placeholders like `{projectRoot}`, `{includeDirs}`, `{sourceFiles}`, `{testFiles}`, and concrete examples only when demonstrating commands.
