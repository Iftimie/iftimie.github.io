CXX := g++
CXXFLAGS := -std=c++17 -I src -I src/interfaces -Wall -Wextra -O2

SRCS := src/CommandUtils.cpp src/interfaces/Globals.cpp test/host_tests.cpp test/mocks/MockEyes.cpp

ifeq ($(OS),Windows_NT)
EXE := .exe
RM := del /Q
else
EXE :=
RM := rm -f
endif

OUT := test/host_tests$(EXE)

.PHONY: all build run clean

all: build

build: $(OUT)

$(OUT): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(OUT)

run: build
	$(OUT)

clean:
	$(RM) test/host_tests test/host_tests.exe
