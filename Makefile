# Default to 32-bit MinGW for backward compatibility
CXX = g++
TARGET = imei.exe

# MinGW-w64 (64-bit) settings
CXX_64 = x86_64-w64-mingw32-g++
TARGET_64 = imei.exe

# MSVC settings
CXX_MSVC = cl.exe
TARGET_MSVC = imei.exe
CFLAGS_MSVC = /W4 /O2 /DUNICODE /D_UNICODE
LDFLAGS_MSVC = user32.lib gdi32.lib imm32.lib shell32.lib

# Original MinGW (32-bit) settings
CXXFLAGS = -Wall -Wextra -std=c++11
LDFLAGS = -static -lgdi32 -luser32 -limm32 -lshell32 -mwindows

SOURCES = IMEIndicator.cpp

# Default target: build 32-bit version
all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

# Target for 64-bit MinGW-w64 build
mingw64: $(TARGET_64)

$(TARGET_64): $(SOURCES)
	$(CXX_64) $(CXXFLAGS) $(SOURCES) -o $(TARGET_64) $(LDFLAGS)

# Target for MSVC build
msvc: $(TARGET_MSVC)

$(TARGET_MSVC): $(SOURCES)
	$(CXX_MSVC) $(CFLAGS_MSVC) $(SOURCES) /link $(LDFLAGS_MSVC) /OUT:$(TARGET_MSVC) /SUBSYSTEM:WINDOWS

clean:
	-del $(TARGET) $(TARGET_64) $(TARGET_MSVC) *.o *.obj *.ilk *.pdb 2>nul

.PHONY: all mingw64 msvc clean
