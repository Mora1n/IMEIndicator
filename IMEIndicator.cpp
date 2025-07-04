#ifndef UNICODE
#define UNICODE
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

#define WM_APP_CHECK_IME (WM_APP + 1)

// --- Constants ---
const UINT_PTR HIDE_TIMER_ID = 1;
const UINT_PTR TIME_TIMER_ID = 2;
const int HIDE_DELAY_MS = 1500;
const int TIME_UPDATE_INTERVAL_MS = 1000;
const int IME_CHECK_DELAY_MS = 30;
const int WINDOW_MARGIN = 10;
const int DEFAULT_FONT_SIZE = 20;
const int DEFAULT_ALPHA_PERCENT = 50;
const int INDICATOR_TEXT_BUFFER_SIZE = 256;

// --- Globals ---
HHOOK g_hKeyboardHook = NULL;
HWND g_hMainWnd = NULL;
HWND g_hIndicatorWnd = NULL;
HFONT g_hIndicatorFont = NULL;
HFONT g_hTimeFont = NULL;
HBRUSH g_hBackgroundBrush = NULL;
wchar_t g_szIndicatorText[INDICATOR_TEXT_BUFFER_SIZE] = L"";
wchar_t g_szTimeText[INDICATOR_TEXT_BUFFER_SIZE] = L"";

int g_indicatorWidth = 100;
int g_indicatorHeight = 50;
int g_fontSize = DEFAULT_FONT_SIZE;
int g_alphaPercent = DEFAULT_ALPHA_PERCENT;

enum IndicatorPosition {
    POS_CENTER, POS_TOPLEFT, POS_TOPRIGHT, POS_BOTTOMLEFT, POS_BOTTOMRIGHT
};
IndicatorPosition g_indicatorPos = POS_BOTTOMRIGHT;

// --- Forward Declarations ---
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK IndicatorWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LowLevelKeyboardProc(int, WPARAM, LPARAM);
void ShowIndicator(const std::wstring&);
std::wstring GetCurrentInputLanguage();
void ParseCommandLine(PWSTR);
void UpdateIndicatorPosition();

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
        HWND_MESSAGE, NULL, hInstance, NULL
    );

    if (!g_hMainWnd) {
        return 1;
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
 * @brief Window procedure for the main hidden window.
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

            g_hIndicatorWnd = CreateWindowExW(
                WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
                INDICATOR_CLASS_NAME, L"Indicator", WS_POPUP,
                0, 0, g_indicatorWidth, g_indicatorHeight,
                NULL, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
            );

            SetLayeredWindowAttributes(g_hIndicatorWnd, 0, (255 * g_alphaPercent) / 100, LWA_ALPHA);

            HDC hdc = GetDC(g_hIndicatorWnd);
            g_hIndicatorFont = CreateFontW(
                -MulDiv(g_fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0, FW_BOLD,
                FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
            );
            g_hTimeFont = CreateFontW(
                -MulDiv(g_fontSize / 2, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0, FW_NORMAL,
                FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
            );
            ReleaseDC(g_hIndicatorWnd, hdc);

            UpdateIndicatorPosition();
            SetTimer(hwnd, TIME_TIMER_ID, TIME_UPDATE_INTERVAL_MS, NULL);
            break;
        }
        case WM_DESTROY: {
            if (g_hKeyboardHook) UnhookWindowsHookEx(g_hKeyboardHook);
            if (g_hIndicatorFont) DeleteObject(g_hIndicatorFont);
            if (g_hTimeFont) DeleteObject(g_hTimeFont);
            if (g_hBackgroundBrush) DeleteObject(g_hBackgroundBrush);
            PostQuitMessage(0);
            break;
        }
        case WM_INPUTLANGCHANGE:
        case WM_APP_CHECK_IME: {
            Sleep(IME_CHECK_DELAY_MS);
            ShowIndicator(GetCurrentInputLanguage());
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

            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            // Select fonts and get text dimensions
            HFONT hOldFont = (HFONT)SelectObject(hdc, g_hIndicatorFont);
            SIZE indicatorSize;
            GetTextExtentPoint32W(hdc, g_szIndicatorText, (int)wcslen(g_szIndicatorText), &indicatorSize);

            SelectObject(hdc, g_hTimeFont);
            SIZE timeSize;
            GetTextExtentPoint32W(hdc, g_szTimeText, (int)wcslen(g_szTimeText), &timeSize);

            // Calculate centered vertical position for the entire text block
            int totalTextHeight = indicatorSize.cy + timeSize.cy;
            int startY = (clientRect.bottom - totalTextHeight) / 2;

            // Define separate rectangles for each text element
            RECT indicatorRect = {clientRect.left, startY, clientRect.right, startY + indicatorSize.cy};
            RECT timeRect = {clientRect.left, startY + indicatorSize.cy, clientRect.right, startY + totalTextHeight};

            // Draw texts in their respective rectangles
            SelectObject(hdc, g_hIndicatorFont);
            DrawTextW(hdc, g_szIndicatorText, -1, &indicatorRect, DT_CENTER | DT_SINGLELINE);

            SelectObject(hdc, g_hTimeFont);
            DrawTextW(hdc, g_szTimeText, -1, &timeRect, DT_CENTER | DT_SINGLELINE);

            SelectObject(hdc, hOldFont); // Restore original font
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
        bool isWinDown = ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) != 0;

        if ((pkbhs->vkCode == VK_SHIFT || pkbhs->vkCode == VK_LSHIFT || pkbhs->vkCode == VK_RSHIFT) &&
            ((!isControlDown && !isAltDown && !isWinDown) ||
             ((pkbhs->flags & LLKHF_ALTDOWN) && !isControlDown && !isWinDown) ||
             (isControlDown && !(pkbhs->flags & LLKHF_ALTDOWN) && !isWinDown))) {
            trigger = true;
        } else if ((pkbhs->vkCode == VK_MENU || pkbhs->vkCode == VK_LMENU || pkbhs->vkCode == VK_RMENU) &&
                   (isShiftDown && !isControlDown && !isWinDown)) {
            trigger = true;
        } else if ((pkbhs->vkCode == VK_CONTROL || pkbhs->vkCode == VK_LCONTROL || pkbhs->vkCode == VK_RCONTROL) &&
                   (isShiftDown && !(pkbhs->flags & LLKHF_ALTDOWN) && !isWinDown)) {
            trigger = true;
        } else if (pkbhs->vkCode == VK_SPACE &&
                   ((isWinDown && !isControlDown && !(pkbhs->flags & LLKHF_ALTDOWN) && !isShiftDown) ||
                    (isControlDown && !(pkbhs->flags & LLKHF_ALTDOWN) && !isWinDown && !isShiftDown))) {
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
    if (!argv) return;

    for (int i = 0; i < argc; ++i) {
        std::wstring arg = argv[i];
        std::wstring key, value;

        size_t eq_pos = arg.find(L'=');
        if (arg.rfind(L"--", 0) == 0 && eq_pos != std::wstring::npos) {
            key = arg.substr(2, eq_pos - 2);
            value = arg.substr(eq_pos + 1);
        } else if ((arg.rfind(L"--", 0) == 0 || arg.rfind(L"-", 0) == 0) && i + 1 < argc) {
            key = arg.substr(arg[1] == L'-' ? 2 : 1);
            value = argv[++i];
        } else {
            continue;
        }

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
    LocalFree(argv);
}

/**
 * @brief Recalculates size and position of the indicator window and updates it.
 */
void UpdateIndicatorPosition() {
    if (!g_hIndicatorWnd) return;

    HDC hdc = GetDC(g_hIndicatorWnd);
    HFONT hOldFont = (HFONT)SelectObject(hdc, g_hIndicatorFont);
    SIZE textSizeIndicator;
    GetTextExtentPoint32W(hdc, g_szIndicatorText, (int)wcslen(g_szIndicatorText), &textSizeIndicator);
    SelectObject(hdc, g_hTimeFont);
    SIZE textSizeTime;
    GetTextExtentPoint32W(hdc, g_szTimeText, (int)wcslen(g_szTimeText), &textSizeTime);
    SelectObject(hdc, hOldFont);
    ReleaseDC(g_hIndicatorWnd, hdc);

    g_indicatorWidth = static_cast<int>(std::max(textSizeIndicator.cx, textSizeTime.cx) * 1.05);
    g_indicatorHeight = static_cast<int>((textSizeIndicator.cy + textSizeTime.cy) * 1.05);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x, y;

    switch (g_indicatorPos) {
        case POS_TOPLEFT:     x = WINDOW_MARGIN; y = WINDOW_MARGIN; break;
        case POS_TOPRIGHT:    x = screenWidth - g_indicatorWidth - WINDOW_MARGIN; y = WINDOW_MARGIN; break;
        case POS_BOTTOMLEFT:  x = WINDOW_MARGIN; y = screenHeight - g_indicatorHeight - WINDOW_MARGIN; break;
        case POS_BOTTOMRIGHT: x = screenWidth - g_indicatorWidth - WINDOW_MARGIN; y = screenHeight - g_indicatorHeight - WINDOW_MARGIN; break;
        case POS_CENTER:
        default:              x = (screenWidth - g_indicatorWidth) / 2; y = (screenHeight - g_indicatorHeight) / 2; break;
    }
    SetWindowPos(g_hIndicatorWnd, NULL, x, y, g_indicatorWidth, g_indicatorHeight, SWP_NOZORDER | SWP_NOACTIVATE);
}

/**
 * @brief Displays the IME indicator window.
 */
void ShowIndicator(const std::wstring& text) {
    if (text == g_szIndicatorText && IsWindowVisible(g_hIndicatorWnd)) {
        SetTimer(g_hMainWnd, HIDE_TIMER_ID, HIDE_DELAY_MS, NULL);
        return;
    }
    if (!g_hIndicatorWnd || !g_hMainWnd) return;

    std::wstring displayText = text.empty() ? L"..." : text;
    wcsncpy(g_szIndicatorText, displayText.c_str(), INDICATOR_TEXT_BUFFER_SIZE - 1);
    g_szIndicatorText[INDICATOR_TEXT_BUFFER_SIZE - 1] = L'\0';

    UpdateIndicatorPosition();

    ShowWindow(g_hIndicatorWnd, SW_SHOWNA);
    InvalidateRect(g_hIndicatorWnd, NULL, TRUE);
    UpdateWindow(g_hIndicatorWnd);

    SetTimer(g_hMainWnd, HIDE_TIMER_ID, HIDE_DELAY_MS, NULL);
}

/**
 * @brief Retrieves the current input language/IME status.
 */
std::wstring GetCurrentInputLanguage() {
    HWND hCurWnd = NULL;
    GUITHREADINFO guiThreadInfo = {};
    guiThreadInfo.cbSize = sizeof(GUITHREADINFO);
    if (GetGUIThreadInfo(0, &guiThreadInfo)) {
        hCurWnd = guiThreadInfo.hwndFocus;
    }
    if (!hCurWnd) {
        hCurWnd = GetForegroundWindow();
    }
    if (!hCurWnd) return L"";

    DWORD dwThreadId = GetWindowThreadProcessId(hCurWnd, NULL);
    HKL hkl = GetKeyboardLayout(dwThreadId);
    LANGID langId = LOWORD(hkl);

    if (PRIMARYLANGID(langId) == LANG_CHINESE) {
        HIMC himc = ImmGetContext(hCurWnd);
        if (himc) {
            DWORD dwConversion, dwSentence;
            if (ImmGetConversionStatus(himc, &dwConversion, &dwSentence)) {
                if (dwConversion & IME_CMODE_ALPHANUMERIC) {
                    ImmReleaseContext(hCurWnd, himc);
                    return L"ENG";
                }
            }
            ImmReleaseContext(hCurWnd, himc);
        }
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
