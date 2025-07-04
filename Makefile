# Default to 32-bit MinGW for backward compatibility
CXX = g++
TARGET = imei.exe

# Directories
BUILD_DIR = build
BIN_DIR = bin

# MinGW-w64 (64-bit) settings
CXX_64 = x86_64-w64-mingw32-g++
TARGET_64 = imei64.exe

# MSVC settings
CXX_MSVC = cl.exe
TARGET_MSVC = imei_msvc.exe
CFLAGS_MSVC = /W4 /O2 /DUNICODE /D_UNICODE /utf-8
LDFLAGS_MSVC = user32.lib gdi32.lib imm32.lib shell32.lib

# Original MinGW (32-bit) settings
CXXFLAGS = -Wall -Wextra -std=c++11
LDFLAGS = -static -lgdi32 -luser32 -limm32 -lshell32 -mwindows

SOURCES = IMEIndicator.cpp
OBJS_MINGW_32 = $(BUILD_DIR)/$(patsubst %.cpp,%_32.o,$(SOURCES))
OBJS_MINGW_64 = $(BUILD_DIR)/$(patsubst %.cpp,%_64.o,$(SOURCES))
OBJS_MSVC = $(BUILD_DIR)/$(SOURCES:.cpp=.obj)

# Default target: build 32-bit version
all: $(BIN_DIR)/$(TARGET)

$(BIN_DIR)/$(TARGET): $(OBJS_MINGW_32) | $(BIN_DIR)
	$(CXX) $(OBJS_MINGW_32) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%_32.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Target for 64-bit MinGW-w64 build
mingw64: $(BIN_DIR)/$(TARGET_64)

$(BIN_DIR)/$(TARGET_64): $(OBJS_MINGW_64) | $(BIN_DIR)
	$(CXX_64) $(OBJS_MINGW_64) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%_64.o: %.cpp | $(BUILD_DIR)
	$(CXX_64) $(CXXFLAGS) -c $< -o $@

VCVARS_CMD = $(if $(VCVARS_PATH),CALL "$(VCVARS_PATH)" &&,)

# Target for MSVC build
msvc: $(BIN_DIR)/$(TARGET_MSVC)

$(BIN_DIR)/$(TARGET_MSVC): $(OBJS_MSVC) | $(BIN_DIR)
	$(VCVARS_CMD) $(CXX_MSVC) $(OBJS_MSVC) /link $(LDFLAGS_MSVC) /OUT:$@ /SUBSYSTEM:WINDOWS

$(BUILD_DIR)/%.obj: %.cpp | $(BUILD_DIR)
	$(VCVARS_CMD) $(CXX_MSVC) $(CFLAGS_MSVC) /c $< /Fo:$@

clean:
	-del $(BIN_DIR)/$(TARGET) $(BIN_DIR)/$(TARGET_64) $(BIN_DIR)/$(TARGET_MSVC) $(BUILD_DIR)\*.o $(BUILD_DIR)\*.obj $(BUILD_DIR)\*.ilk $(BUILD_DIR)\*.pdb 2>nul
	-rmdir $(BUILD_DIR) $(BIN_DIR) 2>nul

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BIN_DIR):
	mkdir $(BIN_DIR)

.PHONY: all mingw64 msvc clean
