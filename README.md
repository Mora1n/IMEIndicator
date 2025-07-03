# IMEIndicator

A lightweight Windows application to display the current Input Method Editor (IME) status on-screen. Useful when the taskbar IME indicator is not visible.

## Features

*   **Clear Indication**: Shows "拼" for Chinese IME and "ENG" for other input methods.
*   **Current Time Display**: Shows the current time.
*   **Configurable**: Customize position, font size, and transparency via command-line arguments.
*   **Low Resource Usage**: Built with pure Win32 API for efficiency.
*   **Hotkey Triggered**: Activates on common IME switching hotkeys (e.g., Shift, Ctrl+Space, Win+Space).

## Usage

Run `imei.exe` with optional arguments:

```bash
imei.exe [OPTIONS]
```

**Options:**

*   `-p, --pos <position>`: Sets display position.
    *   `center` (default), `topleft`, `topright`, `bottomleft`, `bottomright`.
    *   Example: `imei.exe -p topleft` or `imei.exe --pos=topleft`
*   `-s, --size <points>`: Sets font size in points (default: `20`).
    *   Example: `imei.exe -s 36` or `imei.exe --size=36`
*   `-a, --alpha <percentage>`: Sets transparency (0-100, default: `50`).
    *   Example: `imei.exe -a 75` or `imei.exe --alpha=75`

**To Terminate:** Use Task Manager (Ctrl+Shift+Esc), find `imei.exe`, and end the task.

## Building from Source

### Prerequisites

*   A C++ compiler (e.g., MinGW-w64 GCC).

### Compilation

Navigate to the source directory and use `mingw32-make.exe`.

**Using `mingw32-make.exe` (64-bit build):**

```bash
mingw32-make.exe
```

**Using `x86_64-w64-mingw32-g++` directly (64-bit build):**

```bash
x86_64-w64-mingw32-g++ IMEIndicator.cpp -o imei.exe -lgdi32 -luser32 -limm32 -lshell32 -mwindows
```