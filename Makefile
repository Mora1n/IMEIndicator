# IMEIndicator Makefile
# Supports MinGW 32-bit and 64-bit builds

CXX = g++
CXX_64 = x86_64-w64-mingw32-g++

TARGET = imei.exe
TARGET_64 = imei64.exe

BUILD_DIR = build
BIN_DIR = bin

CXXFLAGS = -Wall -Wextra -std=c++11 -O2
LDFLAGS = -static -lgdi32 -luser32 -limm32 -lshell32 -mwindows

SOURCES = IMEIndicator.cpp
OBJS_32 = $(BUILD_DIR)/IMEIndicator_32.o
OBJS_64 = $(BUILD_DIR)/IMEIndicator_64.o

# Default target: build 32-bit version
all: $(BIN_DIR)/$(TARGET)

# 32-bit MinGW build
$(BIN_DIR)/$(TARGET): $(OBJS_32) | $(BIN_DIR)
	$(CXX) $(OBJS_32) -o $@ $(LDFLAGS)

$(OBJS_32): $(SOURCES) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 64-bit MinGW-w64 build
mingw64: $(BIN_DIR)/$(TARGET_64)

$(BIN_DIR)/$(TARGET_64): $(OBJS_64) | $(BIN_DIR)
	$(CXX_64) $(OBJS_64) -o $@ $(LDFLAGS)

$(OBJS_64): $(SOURCES) | $(BUILD_DIR)
	$(CXX_64) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	-del $(BIN_DIR)\$(TARGET) $(BIN_DIR)\$(TARGET_64) $(BUILD_DIR)\*.o 2>nul
	-rmdir $(BUILD_DIR) $(BIN_DIR) 2>nul

# Create directories
$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BIN_DIR):
	mkdir $(BIN_DIR)

.PHONY: all mingw64 clean
