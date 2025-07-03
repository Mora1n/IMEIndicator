#ifndef UNICODE
#define UNICODE
#include <cstdio>
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <windowsx.h>
#include <imm.h>
#include <string>
#include <cwchar>
#include <wchar.h>
#include <shellapi.h>
#include <cstdlib>

HHOOK g_hKeyboardHook = NULL;
HWND g_hMainWnd = NULL;
HWND g_hIndicatorWnd = NULL;
const UINT_PTR HIDE_TIMER_ID = 1;
wchar_t g_szIndicatorText[256] = L"";
HFONT g_hIndicatorFont = NULL;
HFONT g_hTimeFont = NULL;
HBRUSH g_hBackgroundBrush = NULL;
static std::wstring g_lastImeString = L"";
#define WM_APP_CHECK_IME (WM_APP + 1)
const UINT_PTR TIME_TIMER_ID = 2;
wchar_t g_szTimeText[256] = L"";

int g_indicatorWidth = 100;
int g_indicatorHeight = 50;
int g_fontSize = 20;
int g_alphaPercent = 50;
enum IndicatorPosition {
    POS_CENTER,
    POS_TOPLEFT,
    POS_TOPRIGHT,
    POS_BOTTOMLEFT,
    POS_BOTTOMRIGHT
};
IndicatorPosition g_indicatorPos = POS_BOTTOMRIGHT;

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK IndicatorWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
void ShowIndicator(const std::wstring& text);
std::wstring GetCurrentInputLanguage();
void ParseCommandLine(PWSTR pCmdLine);

/**
 * @brief Checks if a virtual key is pressed.
 */
bool IsKeyDown(int vkCode) {
    return (GetAsyncKeyState(vkCode) & 0x8000) != 0;
}

/**
 * @brief Main entry point of the application.
 */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int /*nCmdShow*/) {
    ParseCommandLine(pCmdLine);



    const wchar_t MAIN_CLASS_NAME[] = L"MainHiddenImeListener";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = MAIN_CLASS_NAME;
    RegisterClassW(&wc);

    g_hMainWnd = CreateWindowExW(
        0, MAIN_CLASS_NAME, L"IME Listener", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        HWND_MESSAGE,
        NULL, hInstance, NULL
    );

    if (g_hMainWnd == NULL) {
        return 0;
    }

    g_hKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);
    ShowIndicator(GetCurrentInputLanguage());

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_hKeyboardHook) {
        UnhookWindowsHookEx(g_hKeyboardHook);
    }

    return 0;
}

/**
 * @brief Window procedure for the main hidden window
 */
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            const wchar_t INDICATOR_CLASS_NAME[] = L"ImeIndicatorClass";
            WNDCLASSW wc = {};
            wc.lpfnWndProc = IndicatorWndProc;
            wc.hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
            wc.lpszClassName = INDICATOR_CLASS_NAME;
            g_hBackgroundBrush = CreateSolidBrush(RGB(0, 0, 0));
            wc.hbrBackground = g_hBackgroundBrush;
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            RegisterClassW(&wc);

            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            int x, y;
            int margin = 10;

            switch (g_indicatorPos) {
                case POS_TOPLEFT:
                    x = margin;
                    y = margin;
                    break;
                case POS_TOPRIGHT:
                    x = screenWidth - g_indicatorWidth - margin;
                    y = margin;
                    break;
                case POS_BOTTOMLEFT:
                    x = margin;
                    y = screenHeight - g_indicatorHeight - margin;
                    break;
                case POS_BOTTOMRIGHT:
                    x = screenWidth - g_indicatorWidth - margin;
                    y = screenHeight - g_indicatorHeight - margin;
                    break;
                case POS_CENTER:
                default:
                    x = (screenWidth - g_indicatorWidth) / 2;
                    y = (screenHeight - g_indicatorHeight) / 2;
                    break;
            }

            g_hIndicatorWnd = CreateWindowExW(
                WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
                INDICATOR_CLASS_NAME, L"Indicator", WS_POPUP,
                x, y, g_indicatorWidth, g_indicatorHeight,
                NULL, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
            );

            SetLayeredWindowAttributes(g_hIndicatorWnd, 0, (255 * g_alphaPercent) / 100, LWA_ALPHA);

            HDC hdc = GetDC(g_hIndicatorWnd);
            g_hIndicatorFont = CreateFontW(
                -MulDiv(g_fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72),
                0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
            );
            g_hTimeFont = CreateFontW(
                -MulDiv(g_fontSize / 2, GetDeviceCaps(hdc, LOGPIXELSY), 72),
                0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
            );
            ReleaseDC(g_hIndicatorWnd, hdc);
            SetTimer(hwnd, TIME_TIMER_ID, 1000, NULL); // 1-second timer for time update
            break;
        }

        case WM_DESTROY:{
            if (g_hKeyboardHook) {
                UnhookWindowsHookEx(g_hKeyboardHook);
            }
            if (g_hIndicatorFont) {
                DeleteObject(g_hIndicatorFont);
            }
            if (g_hTimeFont) {
                DeleteObject(g_hTimeFont);
            }
            if (g_hBackgroundBrush) {
                DeleteObject(g_hBackgroundBrush);
            }
            PostQuitMessage(0);
            break;
        }

        case WM_INPUTLANGCHANGE:
        case WM_APP_CHECK_IME: {
            Sleep(30);
            std::wstring lang = GetCurrentInputLanguage();
            ShowIndicator(lang);
            break;
        }

        case WM_TIMER: {
            if (wParam == HIDE_TIMER_ID) {
                ShowWindow(g_hIndicatorWnd, SW_HIDE);
                KillTimer(hwnd, HIDE_TIMER_ID);
            } else if (wParam == TIME_TIMER_ID) {
                SYSTEMTIME st;
                GetLocalTime(&st);
                wsprintfW(g_szTimeText, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
                InvalidateRect(g_hIndicatorWnd, NULL, TRUE);
                UpdateWindow(g_hIndicatorWnd);
            }
            break;
        }

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

/**
 * @brief Window procedure for the IME indicator window.
 */
LRESULT CALLBACK IndicatorWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));

            HFONT hOldFont = (HFONT)SelectObject(hdc, g_hIndicatorFont);

            RECT rect;
            GetClientRect(hwnd, &rect);

            SIZE textSizeIndicator;
            GetTextExtentPoint32W(hdc, g_szIndicatorText, (int)wcslen(g_szIndicatorText), &textSizeIndicator);

            // Draw indicator text
            RECT indicatorTextRect = {rect.left, rect.top, rect.right, rect.bottom}; 
            DrawTextW(hdc, g_szIndicatorText, -1, &indicatorTextRect, DT_CENTER | DT_SINGLELINE);

            // Draw time text below indicator
            HFONT hOldFontTime = (HFONT)SelectObject(hdc, g_hTimeFont);
            RECT timeRect = rect;
            timeRect.top = indicatorTextRect.top + textSizeIndicator.cy;
            DrawTextW(hdc, g_szTimeText, -1, &timeRect, DT_CENTER | DT_SINGLELINE);
            SelectObject(hdc, hOldFontTime);

            SelectObject(hdc, hOldFont);
            EndPaint(hwnd, &ps);
            break;
        }
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

/**
 * @brief Low-level keyboard hook procedure.
 */
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)) {
        KBDLLHOOKSTRUCT* pkbhs = (KBDLLHOOKSTRUCT*)lParam;
        bool trigger = false;

        bool isControlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool isShiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool isAltDown = (GetKeyState(VK_MENU) & 0x8000) != 0;
        bool isWinDown = (GetKeyState(VK_LWIN) & 0x8000) != 0 || (GetKeyState(VK_RWIN) & 0x8000) != 0;

        // Check for Shift hotkeys
        if ((pkbhs->vkCode == VK_SHIFT || pkbhs->vkCode == VK_LSHIFT || pkbhs->vkCode == VK_RSHIFT) &&
            ((!isControlDown && !isAltDown && !isWinDown) || // Plain Shift
             ((pkbhs->flags & LLKHF_ALTDOWN) && !isControlDown && !isWinDown) || // Shift + Alt (Shift released)
             (isControlDown && !(pkbhs->flags & LLKHF_ALTDOWN) && !isWinDown))) // Shift + Ctrl (Shift released)
        {
            trigger = true;
        }
        // Check for Alt hotkeys (when Alt is the released key)
        else if ((pkbhs->vkCode == VK_MENU || pkbhs->vkCode == VK_LMENU || pkbhs->vkCode == VK_RMENU) &&
                 (isShiftDown && !isControlDown && !isWinDown)) // Shift + Alt (Alt released)
        {
            trigger = true;
        }
        // Check for Ctrl hotkeys (when Ctrl is the released key)
        else if ((pkbhs->vkCode == VK_CONTROL || pkbhs->vkCode == VK_LCONTROL || pkbhs->vkCode == VK_RCONTROL) &&
                 (isShiftDown && !(pkbhs->flags & LLKHF_ALTDOWN) && !isWinDown)) // Shift + Ctrl (Ctrl released)
        {
            trigger = true;
        }
        // Check for Space hotkeys (when Space is the released key)
        else if (pkbhs->vkCode == VK_SPACE &&
                 ((isWinDown && !isControlDown && !(pkbhs->flags & LLKHF_ALTDOWN) && !isShiftDown) || // Win + Space
                  (isControlDown && !(pkbhs->flags & LLKHF_ALTDOWN) && !isWinDown && !isShiftDown))) // Ctrl + Space
        {
            trigger = true;
        }

        if (trigger) {
            PostMessage(g_hMainWnd, WM_APP_CHECK_IME, 0, 0);
        }
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

/**
 * @brief Parses command-line arguments.
 */
void ParseCommandLine(PWSTR pCmdLine) {
    int argc;
    LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc);
    if (argv == NULL) return;

    for (int i = 0; i < argc; ++i) {
        std::wstring current_arg = argv[i];
        std::wstring key;
        std::wstring value;
        bool found_arg = false;

        if (current_arg.rfind(L"--", 0) == 0) {
            size_t eq_pos = current_arg.find(L'=', 2);
            if (eq_pos != std::wstring::npos) {
                key = current_arg.substr(2, eq_pos - 2);
                value = current_arg.substr(eq_pos + 1);
                found_arg = true;
            }
        }

        if (!found_arg && (current_arg.rfind(L"--", 0) == 0 || current_arg.rfind(L"-", 0) == 0)) {
            if (i + 1 < argc) {
                key = current_arg.substr(current_arg.rfind(L"-", 0) == 0 ? 1 : 2);
                value = argv[i + 1];
                found_arg = true;
                i++;
            }
        }

        if (found_arg) {
            if (key == L"pos" || key == L"p") {
                if (value == L"topleft") g_indicatorPos = POS_TOPLEFT;
                else if (value == L"topright") g_indicatorPos = POS_TOPRIGHT;
                else if (value == L"bottomleft") g_indicatorPos = POS_BOTTOMLEFT;
                else if (value == L"bottomright") g_indicatorPos = POS_BOTTOMRIGHT;
                else g_indicatorPos = POS_CENTER;
            } else if (key == L"size" || key == L"s") {
                long size = wcstol(value.c_str(), NULL, 10);
                if (size > 0) g_fontSize = (int)size;
            } else if (key == L"alpha" || key == L"a") {
                long alpha = wcstol(value.c_str(), NULL, 10);
                if (alpha >= 0 && alpha <= 100) g_alphaPercent = (int)alpha;
            }
        }
    }
    LocalFree(argv);
}

/**
 * @brief Displays the IME indicator window.
 */
void ShowIndicator(const std::wstring& text) {
    if (text == g_szIndicatorText && IsWindowVisible(g_hIndicatorWnd)) {
        // If the text is the same and the window is already visible, just reset the timer
        KillTimer(g_hMainWnd, HIDE_TIMER_ID);
        SetTimer(g_hMainWnd, HIDE_TIMER_ID, 1500, NULL);
        return;
    }

    std::wstring displayText = text;
    if (displayText.empty()) {
        displayText = L"..."; // Default text if IME string is empty
    }
    if (!g_hIndicatorWnd || !g_hMainWnd) return;

    wcsncpy(g_szIndicatorText, displayText.c_str(), 255);
    g_szIndicatorText[255] = L'\0';

    HDC hdc = GetDC(g_hIndicatorWnd);
    HFONT hOldFont = (HFONT)SelectObject(hdc, g_hIndicatorFont);

    SIZE textSizeIndicator;
    GetTextExtentPoint32W(hdc, g_szIndicatorText, (int)wcslen(g_szIndicatorText), &textSizeIndicator);

    SelectObject(hdc, g_hTimeFont);
    SIZE textSizeTime;
    GetTextExtentPoint32W(hdc, g_szTimeText, (int)wcslen(g_szTimeText), &textSizeTime);

    SelectObject(hdc, hOldFont);
    ReleaseDC(g_hIndicatorWnd, hdc);

    // Use predefined multipliers for size
    g_indicatorWidth = static_cast<int>(std::max(textSizeIndicator.cx, textSizeTime.cx) * 1.05);
    g_indicatorHeight = static_cast<int>((textSizeIndicator.cy + textSizeTime.cy) * 1.05);

    // Recalculate position based on new size
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x, y;
    int margin = 10;

    switch (g_indicatorPos) {
        case POS_TOPLEFT:
            x = margin;
            y = margin;
            break;
        case POS_TOPRIGHT:
            x = screenWidth - g_indicatorWidth - margin;
            y = margin;
            break;
        case POS_BOTTOMLEFT:
            x = margin;
            y = screenHeight - g_indicatorHeight - margin;
            break;
        case POS_BOTTOMRIGHT:
            x = screenWidth - g_indicatorWidth - margin;
            y = screenHeight - g_indicatorHeight - margin;
            break;
        case POS_CENTER:
        default:
            x = (screenWidth - g_indicatorWidth) / 2;
            y = (screenHeight - g_indicatorHeight) / 2;
            break;
    }

    SetWindowPos(g_hIndicatorWnd, NULL, x, y, g_indicatorWidth, g_indicatorHeight, SWP_NOZORDER | SWP_NOACTIVATE);

    ShowWindow(g_hIndicatorWnd, SW_SHOWNA);
    InvalidateRect(g_hIndicatorWnd, NULL, TRUE);
    UpdateWindow(g_hIndicatorWnd);

    KillTimer(g_hMainWnd, HIDE_TIMER_ID);
    SetTimer(g_hMainWnd, HIDE_TIMER_ID, 1500, NULL);
}

/**
 * @brief Retrieves the current input language/IME status.
 */
std::wstring GetCurrentInputLanguage() {
    HWND hCurWnd = GetFocus(); // Get the window with keyboard focus
    if (!hCurWnd) {
        hCurWnd = GetForegroundWindow(); // Fallback to foreground window
    }

    if (!hCurWnd) return L"";

    DWORD dwThreadId = GetWindowThreadProcessId(hCurWnd, NULL);
    HKL hkl = GetKeyboardLayout(dwThreadId);

    LANGID langId = LOWORD(hkl);
    std::wstring currentImeString = L"";

    if (PRIMARYLANGID(langId) == LANG_CHINESE) {
        return L"拼";
    } else {
        return L"ENG";
    }
}

/**
 * @brief Standard C++ main entry point wrapper.
 */
int main() {
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (NULL == argv) { return 0; }
    std::wstring cmdLine;
    for (int i = 1; i < argc; ++i) {
        cmdLine += argv[i];
        if (i < argc - 1) cmdLine += L" ";
    }
    int result = wWinMain(GetModuleHandle(NULL), NULL, &cmdLine[0], SW_SHOWNORMAL);
    LocalFree(argv);
    return result;
}