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

*   **For MinGW:** A MinGW (for 32-bit) or MinGW-w64 (for 64-bit) toolchain.
*   **For MSVC:** Visual Studio or Visual Studio Build Tools. If not using a "Developer Command Prompt for VS", you may need to manually run `"C:\Program Files\Microsoft Visual Studio\<version>\<edition>\VC\Auxiliary\Build\vcvars64.bat"` (adjust path as needed) before compiling.

### Compilation

Open a terminal in the project directory and use `mingw32-make` with the desired target. Executables will be generated in the `bin/` directory, and intermediate files in the `build/` directory.

*   **Default (32-bit MinGW):**
    ```bash
    mingw32-make
    ```
    This creates `bin/imei.exe`.

*   **64-bit (MinGW-w64):**
    ```bash
    mingw32-make mingw64
    ```
    This creates `bin/imei64.exe`. Requires the `x86_64-w64-mingw32-g++` compiler in your PATH.

*   **64-bit (MSVC):**
    ```bash
    mingw32-make msvc VCVARS_PATH="C:\Program Files\Microsoft\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    ```
    This creates `bin/imei_msvc.exe`. Requires the `cl.exe` compiler. Replace the `VCVARS_PATH` with your actual `vcvars64.bat` path.


*   **Clean:**
    ```bash
    mingw32-make clean
    ```
    This removes all build artifacts from `bin/` and `build/`.