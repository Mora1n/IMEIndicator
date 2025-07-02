# IME Indicator for Windows 11

## Purpose

This lightweight C++ application provides an on-screen indicator for the current input method (IME) or keyboard layout, especially useful when the taskbar is hidden. It aims to solve the problem of not knowing whether Microsoft Pinyin is active (and in Chinese or English mode) or if an English keyboard layout is in use.

## Features

*   **Low Resource Usage**: Built with pure Win32 API for efficiency, optimized for performance and minimal memory footprint.
*   **Clear Indication**: Displays "拼" for Microsoft Pinyin (Chinese/English mode) and "ENG" for Microsoft Pinyin (English mode).
*   **Configurable Display**: Customize font size, transparency, and on-screen position.
*   **Specific Hotkey Triggering**: Activates only on common IME switching hotkeys (Shift, Shift+Alt, Shift+Ctrl, Win+Space, Ctrl+Space).
*   **Improved Startup Reliability**: Ensures the indicator window is visible immediately upon launch, even without initial IME changes.

## Usage

Run `ime_indicator.exe` with optional command-line arguments.

```bash
ime_indicator.exe [OPTIONS]
```

**Options:**

*   `--pos <position>` or `-p <position>`: Sets the display position.
    *   `center` (default), `topleft`, `topright`, `bottomleft`, `bottomright`.
    *   Example: `ime_indicator.exe --pos topleft` or `ime_indicator.exe -p topleft`

*   `--size <points>` or `-s <points>`: Sets the font size in points.
    *   Default: `20`
    *   Example: `ime_indicator.exe --size 36` or `ime_indicator.exe -s 36`

*   `--alpha <percentage>` or `-a <percentage>`: Sets transparency (0-100).
    *   Default: `50`
    *   Example: `ime_indicator.exe --alpha 75` or `ime_indicator.exe -a 75`

**Example Combinations:**

```bash
# Run with default settings
start "" "ime_indicator.exe"

# Run with 36pt font, 75% transparency, at the bottom-right
# Note: The empty double quotes "" are crucial for passing arguments correctly when using 'start' in PowerShell/CMD.
start "" "ime_indicator.exe --size=36 --alpha=75 --pos=bottomright"
start "" "ime_indicator.exe -s 36 -a 75 -p bottomright"
```

**To Terminate:** Use Task Manager (Ctrl+Shift+Esc), find `ime_indicator.exe`, and click "End task".

## Building from Source

### Prerequisites

*   A C++ compiler (e.g., MinGW-w64 GCC).

### Compilation

Navigate to the source directory and use `mingw32-make.exe` (if installed) or `g++` directly.

**Using `mingw32-make.exe`:**

```bash
mingw32-make.exe
```

**Using `g++` directly:**

```bash
g++ ime_indicator.cpp -o ime_indicator.exe -lgdi32 -luser32 -limm32 -lshell32 -mwindows
```