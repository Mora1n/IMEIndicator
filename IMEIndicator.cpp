#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <imm.h>
#include <shellapi.h>
#include <string>
#include <algorithm>

// ============================================================================
// DWM API Definitions (for MinGW compatibility)
// ============================================================================

#ifndef DWMWA_NCRENDERING_POLICY
#define DWMWA_NCRENDERING_POLICY 2
#endif

#ifndef DWMNCRP_ENABLED
#define DWMNCRP_ENABLED 2
#endif

typedef struct _MARGINS {
    int cxLeftWidth, cxRightWidth, cyTopHeight, cyBottomHeight;
} MARGINS;

typedef HRESULT (WINAPI *DwmSetWindowAttributeProc)(HWND, DWORD, LPCVOID, DWORD);
typedef HRESULT (WINAPI *DwmExtendFrameIntoClientAreaProc)(HWND, const MARGINS*);

// ============================================================================
// Constants
// ============================================================================

#define WM_APP_CHECK_IME (WM_APP + 1)

constexpr UINT_PTR HIDE_TIMER_ID = 1;
constexpr UINT_PTR TIME_TIMER_ID = 2;
constexpr int HIDE_DELAY_MS = 1500;
constexpr int TIME_UPDATE_MS = 1000;
constexpr int WINDOW_MARGIN = 10;
constexpr int DEFAULT_FONT_SIZE = 20;
constexpr int DEFAULT_ALPHA = 50;
constexpr int DEFAULT_RADIUS = 10;
constexpr int TEXT_BUFFER_SIZE = 256;
constexpr int MOUSE_OFFSET = 20;

// ============================================================================
// Global Variables
// ============================================================================

// Window handles and resources
static HHOOK g_hKeyboardHook = nullptr;
static HWND g_hMainWnd = nullptr;
static HWND g_hIndicatorWnd = nullptr;
static HFONT g_hIndicatorFont = nullptr;
static HFONT g_hTimeFont = nullptr;
static HMODULE g_hDwmapi = nullptr;

// DWM function pointers
static DwmSetWindowAttributeProc g_pDwmSetWindowAttribute = nullptr;
static DwmExtendFrameIntoClientAreaProc g_pDwmExtendFrameIntoClientArea = nullptr;

// Display text
static wchar_t g_szIndicatorText[TEXT_BUFFER_SIZE] = L"";
static wchar_t g_szTimeText[TEXT_BUFFER_SIZE] = L"";

// Window dimensions
static int g_indicatorWidth = 100;
static int g_indicatorHeight = 50;
static int g_fixedWidth = 0;
static int g_fixedHeight = 0;
static int g_screenWidth = 0;
static int g_screenHeight = 0;

// Configuration
static int g_fontSize = DEFAULT_FONT_SIZE;
static int g_alphaPercent = DEFAULT_ALPHA;
static int g_cornerRadius = DEFAULT_RADIUS;
static COLORREF g_bgColor = RGB(0x22, 0x21, 0x2c);  // #22212c
static COLORREF g_textColor = RGB(0x95, 0x80, 0xff); // #9580ff
static bool g_showTime = true;
static bool g_enableShadow = true;

enum Position { CENTER, TOPLEFT, TOPRIGHT, BOTTOMLEFT, BOTTOMRIGHT, MOUSE };
static Position g_position = MOUSE;

// ============================================================================
// Forward Declarations
// ============================================================================

LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK IndicatorWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK KeyboardProc(int, WPARAM, LPARAM);

static void InitDwmApi();
static void CacheScreenMetrics();
static void CalculateFixedSize();
static void ApplyRoundedCorners(HWND);
static void ApplyDropShadow(HWND);
static void UpdatePosition();
static void ShowIndicator(const std::wstring&);
static void ParseCommandLine(PWSTR);
static std::wstring GetCurrentIME();
static COLORREF ParseColor(const std::wstring&);
static bool IsIMESwitchKey(DWORD, bool, bool, bool, bool);

// ============================================================================
// Main Entry Point
// ============================================================================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int) {
    ParseCommandLine(pCmdLine);
    CacheScreenMetrics();
    InitDwmApi();

    // Register main window class
    WNDCLASSW wc = {};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"IMEIndicatorMain";
    RegisterClassW(&wc);

    // Create hidden message-only window
    g_hMainWnd = CreateWindowExW(0, L"IMEIndicatorMain", L"IME Listener",
        WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr);

    if (!g_hMainWnd) {
        if (g_hDwmapi) FreeLibrary(g_hDwmapi);
        return 1;
    }

    // Install keyboard hook
    g_hKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);
    ShowIndicator(GetCurrentIME());

    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    if (g_hKeyboardHook) UnhookWindowsHookEx(g_hKeyboardHook);
    if (g_hDwmapi) FreeLibrary(g_hDwmapi);
    return 0;
}

// ============================================================================
// Window Procedures
// ============================================================================

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Register indicator window class
            WNDCLASSW wc = {};
            wc.lpfnWndProc = IndicatorWndProc;
            wc.hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
            wc.lpszClassName = L"IMEIndicator";
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            RegisterClassW(&wc);

            // Create indicator window
            g_hIndicatorWnd = CreateWindowExW(
                WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
                L"IMEIndicator", L"", WS_POPUP, 0, 0, g_indicatorWidth, g_indicatorHeight,
                nullptr, nullptr, wc.hInstance, nullptr);

            SetLayeredWindowAttributes(g_hIndicatorWnd, 0, (255 * g_alphaPercent) / 100, LWA_ALPHA);
            ApplyDropShadow(g_hIndicatorWnd);

            // Create fonts
            HDC hdc = GetDC(g_hIndicatorWnd);
            int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
            g_hIndicatorFont = CreateFontW(-MulDiv(g_fontSize, dpi, 72), 0, 0, 0, FW_BOLD,
                FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            if (g_showTime) {
                g_hTimeFont = CreateFontW(-MulDiv(g_fontSize / 2, dpi, 72), 0, 0, 0, FW_NORMAL,
                    FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            }
            ReleaseDC(g_hIndicatorWnd, hdc);

            CalculateFixedSize();
            UpdatePosition();

            if (g_showTime) SetTimer(hwnd, TIME_TIMER_ID, TIME_UPDATE_MS, nullptr);
            break;
        }

        case WM_DESTROY:
            if (g_hIndicatorFont) DeleteObject(g_hIndicatorFont);
            if (g_hTimeFont) DeleteObject(g_hTimeFont);
            PostQuitMessage(0);
            break;

        case WM_INPUTLANGCHANGE:
        case WM_APP_CHECK_IME:
            ShowIndicator(GetCurrentIME());
            break;

        case WM_TIMER:
            if (wParam == HIDE_TIMER_ID) {
                ShowWindow(g_hIndicatorWnd, SW_HIDE);
                KillTimer(hwnd, HIDE_TIMER_ID);
            } else if (wParam == TIME_TIMER_ID && g_showTime) {
                SYSTEMTIME st;
                GetLocalTime(&st);
                wsprintfW(g_szTimeText, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
                if (IsWindowVisible(g_hIndicatorWnd)) {
                    InvalidateRect(g_hIndicatorWnd, nullptr, TRUE);
                }
            }
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK IndicatorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wParam;
            RECT rect;
            GetClientRect(hwnd, &rect);
            HBRUSH brush = CreateSolidBrush(g_bgColor);
            FillRect(hdc, &rect, brush);
            DeleteObject(brush);
            return 1;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, g_textColor);

            RECT rect;
            GetClientRect(hwnd, &rect);
            HFONT oldFont = (HFONT)SelectObject(hdc, g_hIndicatorFont);

            if (g_showTime) {
                SIZE szInd, szTime;
                GetTextExtentPoint32W(hdc, g_szIndicatorText, wcslen(g_szIndicatorText), &szInd);
                SelectObject(hdc, g_hTimeFont);
                GetTextExtentPoint32W(hdc, g_szTimeText, wcslen(g_szTimeText), &szTime);

                int totalH = szInd.cy + szTime.cy;
                int startY = (rect.bottom - totalH) / 2;

                RECT r1 = {rect.left, startY, rect.right, startY + szInd.cy};
                RECT r2 = {rect.left, startY + szInd.cy, rect.right, startY + totalH};

                SelectObject(hdc, g_hIndicatorFont);
                DrawTextW(hdc, g_szIndicatorText, -1, &r1, DT_CENTER | DT_SINGLELINE);
                SelectObject(hdc, g_hTimeFont);
                DrawTextW(hdc, g_szTimeText, -1, &r2, DT_CENTER | DT_SINGLELINE);
            } else {
                DrawTextW(hdc, g_szIndicatorText, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            SelectObject(hdc, oldFont);
            EndPaint(hwnd, &ps);
            break;
        }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)) {
        auto* kb = (KBDLLHOOKSTRUCT*)lParam;
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        bool win = ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) != 0;

        if (IsIMESwitchKey(kb->vkCode, ctrl, shift, alt, win)) {
            PostMessage(g_hMainWnd, WM_APP_CHECK_IME, 0, 0);
        }
    }
    return CallNextHookEx(g_hKeyboardHook, code, wParam, lParam);
}

// ============================================================================
// Helper Functions
// ============================================================================

void InitDwmApi() {
    g_hDwmapi = LoadLibraryW(L"dwmapi.dll");
    if (g_hDwmapi) {
        g_pDwmSetWindowAttribute = (DwmSetWindowAttributeProc)
            GetProcAddress(g_hDwmapi, "DwmSetWindowAttribute");
        g_pDwmExtendFrameIntoClientArea = (DwmExtendFrameIntoClientAreaProc)
            GetProcAddress(g_hDwmapi, "DwmExtendFrameIntoClientArea");
    }
}

void CacheScreenMetrics() {
    g_screenWidth = GetSystemMetrics(SM_CXSCREEN);
    g_screenHeight = GetSystemMetrics(SM_CYSCREEN);
}

void CalculateFixedSize() {
    if (!g_hIndicatorWnd || !g_hIndicatorFont) return;

    HDC hdc = GetDC(g_hIndicatorWnd);
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hIndicatorFont);

    const wchar_t* texts[] = {
        L"拼", L"注", L"中", L"ENG", L"あ", L"ア", L"A", L"日",
        L"한", L"Việt", L"ไทย", L"عر", L"עב", L"РУС", L"हिं", L"..."
    };

    int maxW = 0, maxH = 0;
    for (auto text : texts) {
        SIZE sz;
        GetTextExtentPoint32W(hdc, text, wcslen(text), &sz);
        maxW = std::max(maxW, (int)sz.cx);
        maxH = std::max(maxH, (int)sz.cy);
    }

    int timeH = 0;
    if (g_showTime && g_hTimeFont) {
        SelectObject(hdc, g_hTimeFont);
        SIZE sz;
        GetTextExtentPoint32W(hdc, L"00:00:00", 8, &sz);
        maxW = std::max(maxW, (int)sz.cx);
        timeH = sz.cy;
    }

    SelectObject(hdc, oldFont);
    ReleaseDC(g_hIndicatorWnd, hdc);

    g_fixedWidth = (int)(maxW * 1.3);
    g_fixedHeight = g_showTime ? (int)((maxH + timeH) * 1.3) : (int)(maxH * 1.6);
}

void ApplyRoundedCorners(HWND hwnd) {
    if (g_cornerRadius > 0) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        HRGN rgn = CreateRoundRectRgn(0, 0, rect.right, rect.bottom,
            g_cornerRadius * 2, g_cornerRadius * 2);
        SetWindowRgn(hwnd, rgn, FALSE);
    }
}

void ApplyDropShadow(HWND hwnd) {
    if (g_enableShadow && g_pDwmSetWindowAttribute && g_pDwmExtendFrameIntoClientArea) {
        DWORD policy = DWMNCRP_ENABLED;
        g_pDwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
        MARGINS margins = {1, 1, 1, 1};
        g_pDwmExtendFrameIntoClientArea(hwnd, &margins);
    }
}

void UpdatePosition() {
    if (!g_hIndicatorWnd) return;

    if (g_fixedWidth > 0 && g_fixedHeight > 0) {
        g_indicatorWidth = g_fixedWidth;
        g_indicatorHeight = g_fixedHeight;
    }

    int x, y;
    switch (g_position) {
        case TOPLEFT:
            x = WINDOW_MARGIN;
            y = WINDOW_MARGIN;
            break;
        case TOPRIGHT:
            x = g_screenWidth - g_indicatorWidth - WINDOW_MARGIN;
            y = WINDOW_MARGIN;
            break;
        case BOTTOMLEFT:
            x = WINDOW_MARGIN;
            y = g_screenHeight - g_indicatorHeight - WINDOW_MARGIN;
            break;
        case BOTTOMRIGHT:
            x = g_screenWidth - g_indicatorWidth - WINDOW_MARGIN;
            y = g_screenHeight - g_indicatorHeight - WINDOW_MARGIN;
            break;
        case MOUSE: {
            POINT pt;
            GetCursorPos(&pt);
            x = pt.x + MOUSE_OFFSET;
            y = pt.y + MOUSE_OFFSET;
            if (x + g_indicatorWidth > g_screenWidth) x = pt.x - g_indicatorWidth - MOUSE_OFFSET;
            if (y + g_indicatorHeight > g_screenHeight) y = pt.y - g_indicatorHeight - MOUSE_OFFSET;
            x = std::max(0, std::min(x, g_screenWidth - g_indicatorWidth));
            y = std::max(0, std::min(y, g_screenHeight - g_indicatorHeight));
            break;
        }
        default: // CENTER
            x = (g_screenWidth - g_indicatorWidth) / 2;
            y = (g_screenHeight - g_indicatorHeight) / 2;
    }

    SetWindowPos(g_hIndicatorWnd, nullptr, x, y, g_indicatorWidth, g_indicatorHeight,
        SWP_NOZORDER | SWP_NOACTIVATE);
    ApplyRoundedCorners(g_hIndicatorWnd);
}

void ShowIndicator(const std::wstring& text) {
    if (!g_hIndicatorWnd || !g_hMainWnd) return;

    const wchar_t* str = text.empty() ? L"..." : text.c_str();
    wcsncpy(g_szIndicatorText, str, TEXT_BUFFER_SIZE - 1);
    g_szIndicatorText[TEXT_BUFFER_SIZE - 1] = L'\0';

    UpdatePosition();
    ShowWindow(g_hIndicatorWnd, SW_SHOWNA);
    InvalidateRect(g_hIndicatorWnd, nullptr, TRUE);
    SetTimer(g_hMainWnd, HIDE_TIMER_ID, HIDE_DELAY_MS, nullptr);
}

COLORREF ParseColor(const std::wstring& hex) {
    std::wstring color = hex;
    if (!color.empty() && color[0] == L'#') color = color.substr(1);
    if (color.length() != 6) return RGB(0, 0, 0);

    wchar_t* end;
    unsigned long val = wcstoul(color.c_str(), &end, 16);
    if (*end != L'\0') return RGB(0, 0, 0);

    return RGB((val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF);
}

void ParseCommandLine(PWSTR cmdLine) {
    int argc;
    LPWSTR* argv = CommandLineToArgvW(cmdLine, &argc);
    if (!argv) return;

    for (int i = 0; i < argc; ++i) {
        std::wstring arg = argv[i];

        if (arg == L"--show-time" || arg == L"-t") {
            g_showTime = true;
            continue;
        }
        if (arg == L"--no-shadow") {
            g_enableShadow = false;
            continue;
        }

        std::wstring key, value;
        size_t eq = arg.find(L'=');

        if (arg.rfind(L"--", 0) == 0 && eq != std::wstring::npos) {
            key = arg.substr(2, eq - 2);
            value = arg.substr(eq + 1);
        } else if ((arg.rfind(L"--", 0) == 0 || arg.rfind(L"-", 0) == 0) && i + 1 < argc) {
            key = arg.substr(arg[1] == L'-' ? 2 : 1);
            value = argv[++i];
        } else {
            continue;
        }

        if (key == L"pos" || key == L"p") {
            if (value == L"topleft") g_position = TOPLEFT;
            else if (value == L"topright") g_position = TOPRIGHT;
            else if (value == L"bottomleft") g_position = BOTTOMLEFT;
            else if (value == L"bottomright") g_position = BOTTOMRIGHT;
            else if (value == L"mouse") g_position = MOUSE;
            else g_position = CENTER;
        } else if (key == L"size" || key == L"s") {
            int size = wcstol(value.c_str(), nullptr, 10);
            if (size > 0) g_fontSize = size;
        } else if (key == L"alpha" || key == L"a") {
            int alpha = wcstol(value.c_str(), nullptr, 10);
            if (alpha >= 0 && alpha <= 100) g_alphaPercent = alpha;
        } else if (key == L"radius" || key == L"r") {
            int radius = wcstol(value.c_str(), nullptr, 10);
            if (radius >= 0) g_cornerRadius = radius;
        } else if (key == L"bgcolor" || key == L"bg") {
            g_bgColor = ParseColor(value);
        } else if (key == L"textcolor" || key == L"fg" || key == L"tc") {
            g_textColor = ParseColor(value);
        }
    }
    LocalFree(argv);
}

bool IsIMESwitchKey(DWORD vk, bool ctrl, bool shift, bool alt, bool win) {
    // Shift (alone, Shift+Alt, Shift+Ctrl)
    if (vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT) {
        return (!ctrl && !alt && !win) || (alt && !ctrl && !win) || (ctrl && !alt && !win);
    }
    // Alt (with Shift)
    if ((vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU) && shift && !ctrl && !win) {
        return true;
    }
    // Ctrl (with Shift)
    if ((vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL) && shift && !alt && !win) {
        return true;
    }
    // Space (Win+Space or Ctrl+Space)
    if (vk == VK_SPACE) {
        return (win && !ctrl && !alt && !shift) || (ctrl && !alt && !win && !shift);
    }
    return false;
}

std::wstring GetCurrentIME() {
    HWND hwnd = nullptr;
    GUITHREADINFO info = {};
    info.cbSize = sizeof(info);
    if (GetGUIThreadInfo(0, &info)) hwnd = info.hwndFocus;
    if (!hwnd) hwnd = GetForegroundWindow();
    if (!hwnd) return L"";

    HKL hkl = GetKeyboardLayout(GetWindowThreadProcessId(hwnd, nullptr));
    LANGID langId = LOWORD(hkl);
    WORD primary = PRIMARYLANGID(langId);
    WORD sub = SUBLANGID(langId);

    HIMC himc = ImmGetContext(hwnd);
    DWORD conv = 0, sent = 0;
    bool hasIme = himc && ImmGetConversionStatus(himc, &conv, &sent);

    std::wstring result;
    switch (primary) {
        case LANG_CHINESE:
            if (hasIme && (conv & IME_CMODE_ALPHANUMERIC)) {
                result = L"ENG";
            } else if (sub == SUBLANG_CHINESE_SIMPLIFIED || langId == 0x0804) {
                result = L"拼";
            } else if (sub == SUBLANG_CHINESE_TRADITIONAL || langId == 0x0404 || langId == 0x0C04) {
                result = L"注";
            } else {
                result = L"中";
            }
            break;

        case LANG_JAPANESE:
            if (hasIme) {
                if (conv & IME_CMODE_ALPHANUMERIC) result = L"A";
                else if (conv & IME_CMODE_KATAKANA) result = L"ア";
                else if (conv & IME_CMODE_NATIVE) result = L"あ";
                else result = L"日";
            } else {
                result = L"日";
            }
            break;

        case LANG_KOREAN:
            result = (hasIme && (conv & IME_CMODE_NATIVE)) ? L"한" : L"ENG";
            break;

        case LANG_VIETNAMESE:
            result = (hasIme && (conv & IME_CMODE_NATIVE)) ? L"Việt" : L"ENG";
            break;

        case LANG_THAI:
            result = (hasIme && (conv & IME_CMODE_NATIVE)) ? L"ไทย" : L"ENG";
            break;

        case LANG_ARABIC:
            result = L"عر";
            break;

        case LANG_HEBREW:
            result = L"עב";
            break;

        case LANG_RUSSIAN:
            result = L"РУС";
            break;

        case LANG_HINDI:
            result = L"हिं";
            break;

        default:
            if (primary == LANG_ENGLISH) {
                result = L"ENG";
            } else {
                wchar_t name[32];
                if (GetLocaleInfoW(MAKELCID(langId, SORT_DEFAULT), LOCALE_SABBREVLANGNAME, name, 32)) {
                    result = name;
                    if (result.length() > 4) result = result.substr(0, 4);
                } else {
                    result = L"ENG";
                }
            }
    }

    if (himc) ImmReleaseContext(hwnd, himc);
    return result;
}

// ============================================================================
// Standard main() wrapper
// ============================================================================

int main() {
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return 0;

    std::wstring cmdLine;
    for (int i = 1; i < argc; ++i) {
        cmdLine += argv[i];
        if (i < argc - 1) cmdLine += L" ";
    }

    int result = wWinMain(GetModuleHandle(nullptr), nullptr, &cmdLine[0], SW_SHOWNORMAL);
    LocalFree(argv);
    return result;
}
