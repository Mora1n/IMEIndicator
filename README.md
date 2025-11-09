# IMEIndicator

A lightweight Windows application to display the current Input Method Editor (IME) status on-screen. Useful when the taskbar IME indicator is not visible, especially in fullscreen applications.

## Features

*   **Multi-Language Support**: Detects and displays status for multiple input methods:
    *   **Chinese**: 拼 (Pinyin/Simplified), 注 (Zhuyin/Traditional), ENG (Alphanumeric)
    *   **Japanese**: あ (Hiragana), ア (Katakana), A (Romaji)
    *   **Korean**: 한 (Hangul), ENG (English)
    *   **Vietnamese**: Việt (Vietnamese), ENG (English)
    *   **Thai**: ไทย (Thai), ENG (English)
    *   **Other languages**: Arabic, Hebrew, Russian, Hindi, and more
*   **Modern Visual Effects**:
    *   Rounded corner borders for a modern look
    *   Optional drop shadow effect (disabled by default)
    *   Customizable transparency (80% by default)
    *   **Dracula theme colors** by default (customizable)
*   **Flexible Positioning**:
    *   Fixed positions: center, topleft, topright, bottomleft, bottomright
    *   **Mouse position mode** (default): Display at cursor location when IME changes
*   **Optional Time Display**: Show current time alongside IME status (disabled by default)
*   **Customizable Colors**: Support for custom background and text colors via hex codes
*   **Low Resource Usage**: Built with pure Win32 API for efficiency
*   **Hotkey Triggered**: Activates on common IME switching hotkeys (e.g., Shift, Ctrl+Space, Win+Space)
*   **Auto-hide**: Indicator disappears after 1.5 seconds of inactivity
*   **Instant Response**: No animation delays for immediate feedback

## Usage

Run `imei.exe` with optional arguments:

```bash
imei.exe [OPTIONS]
```

### Command-Line Options

**Position & Display:**

*   `-p, --pos <position>`: Sets display position (default: `mouse`)
    *   Values: `center`, `topleft`, `topright`, `bottomleft`, `bottomright`, `mouse`
    *   `mouse` mode displays the indicator at cursor position when IME changes
    *   Example: `imei.exe -p topleft` or `imei.exe --pos=bottomright`

**Appearance:**

*   `-s, --size <points>`: Sets font size in points (default: `20`)
    *   Example: `imei.exe -s 36` or `imei.exe --size=36`
*   `-a, --alpha <percentage>`: Sets transparency (0-100, default: `80`)
    *   0 = fully transparent, 100 = fully opaque
    *   Example: `imei.exe -a 75` or `imei.exe --alpha=75`
*   `-r, --radius <pixels>`: Sets corner radius in pixels (default: `10`)
    *   Set to 0 for sharp corners
    *   Example: `imei.exe -r 15` or `imei.exe --radius=0`

**Colors (Dracula theme by default):**

*   `--bgcolor <hex>`, `--bg <hex>`: Sets background color (default: `282a36`)
    *   Accepts 6-digit hex color codes with or without `#`
    *   Example: `imei.exe --bgcolor=ff79c6` or `imei.exe --bg=#bd93f9`
*   `--textcolor <hex>`, `--fg <hex>`, `--tc <hex>`: Sets text color (default: `f8f8f2`)
    *   Accepts 6-digit hex color codes with or without `#`
    *   Example: `imei.exe --textcolor=8be9fd` or `imei.exe --fg=50fa7b`

**Features:**

*   `-t, --show-time`: Enable time display alongside IME status (disabled by default)
    *   Use this flag to show current time below the IME indicator
    *   Example: `imei.exe --show-time`
*   `--no-shadow`: Disable drop shadow effect (already disabled by default)
    *   Example: `imei.exe --no-shadow`

### Usage Examples

```bash
# Default settings (mouse position, Dracula theme, 20pt, 80% opacity, no time, no shadow)
imei.exe

# Fixed position at bottom-right corner
imei.exe -p bottomright

# Minimal, clean indicator at top-left
imei.exe -p topleft -s 24 -a 70

# Large, opaque indicator at center with sharp corners
imei.exe -p center -s 48 -a 90 -r 0

# Custom Dracula purple theme
imei.exe --bgcolor=22212c --textcolor=9580ff

# Solarized Dark theme
imei.exe --bg=002b36 --fg=839496

# Monokai theme with custom settings
imei.exe --bgcolor=272822 --textcolor=f8f8f2 -s 24 -a 80

# Nord theme at mouse position
imei.exe --bg=2e3440 --fg=d8dee9 --pos=mouse

# Combine multiple options
imei.exe -p topright -s 30 -a 60 -r 20 --bgcolor=44475a --textcolor=50fa7b
```

### Popular Color Themes

**Dracula (default):**
```bash
imei.exe --bgcolor=22212c --textcolor=9580ff
```

**Other Popular Themes:**
```bash
# Solarized Dark
imei.exe --bg=002b36 --fg=839496

# Monokai
imei.exe --bg=272822 --fg=f8f8f2

# Nord
imei.exe --bg=2e3440 --fg=d8dee9

# Gruvbox Dark
imei.exe --bg=282828 --fg=ebdbb2

# One Dark
imei.exe --bg=282c34 --fg=abb2bf

# Tokyo Night
imei.exe --bg=1a1b26 --fg=c0caf5
```

**To Terminate:** Use Task Manager (Ctrl+Shift+Esc), find `imei.exe`, and end the task.

## Building from Source

### Prerequisites

*   **MinGW** (for 32-bit) or **MinGW-w64** (for 64-bit) toolchain

### Compilation

Open a terminal in the project directory and use `mingw32-make`. Executables will be generated in the `bin/` directory.

**32-bit build (default):**
```bash
mingw32-make
```
Creates `bin/imei.exe`

**64-bit build:**
```bash
mingw32-make mingw64
```
Creates `bin/imei64.exe` (requires `x86_64-w64-mingw32-g++` in PATH)

**Clean build artifacts:**
```bash
mingw32-make clean
```
Removes all files from `bin/` and `build/` directories