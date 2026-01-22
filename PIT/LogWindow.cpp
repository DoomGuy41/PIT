#include "LogWindow.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <windows.h>

static HWND hLogWnd = nullptr;
static HWND hEdit = nullptr;
static std::wstring g_LogPath = L"C:\\TEMP\\IntuneFix.log";

// Use a stock font for the edit control (do not delete stock fonts)
static HFONT hLogFont = nullptr;

static LRESULT CALLBACK LogWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_TIMER:
        UpdateLogWindow();
        return 0;

    case WM_CLOSE:
        CloseLogWindow();
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void RegisterLogWindowClass()
{
    static bool registered = false;
    if (registered)
        return;

    WNDCLASSW wc{};
    wc.lpfnWndProc = LogWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"PIT_LogWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);
    registered = true;
}


// Read the log file robustly and convert to UTF-16 for the edit control.
// Supports UTF-8 (with or without BOM) and UTF-16 little-endian (with BOM).
static void LoadLog()
{
    if (!hEdit)
        return;

    std::ifstream in(g_LogPath, std::ios::binary);
    if (!in.good())
        return;

    std::vector<char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    if (data.empty())
        return;

    // Detect BOMs
    std::wstring text;
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(data.data());
    size_t len = data.size();

    if (len >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE)
    {
        // UTF-16 LE with BOM
        size_t wlen = (len - 2) / 2;
        if (wlen > 0)
        {
            const wchar_t* wptr = reinterpret_cast<const wchar_t*>(data.data() + 2);
            text.assign(wptr, wptr + wlen);
        }
    }
    else if (len >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF)
    {
        // UTF-8 with BOM
        int needed = MultiByteToWideChar(CP_UTF8, 0, data.data() + 3, static_cast<int>(len - 3), nullptr, 0);
        if (needed > 0)
        {
            text.resize(needed);
            MultiByteToWideChar(CP_UTF8, 0, data.data() + 3, static_cast<int>(len - 3), &text[0], needed);
        }
    }
    else
    {
        // No BOM: try UTF-8 first (most tools produce UTF-8), fall back to ANSI
        int needed = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, data.data(), static_cast<int>(len), nullptr, 0);
        if (needed > 0)
        {
            text.resize(needed);
            MultiByteToWideChar(CP_UTF8, 0, data.data(), static_cast<int>(len), &text[0], needed);
        }
        else
        {
            // Fallback to ANSI code page
            needed = MultiByteToWideChar(CP_ACP, 0, data.data(), static_cast<int>(len), nullptr, 0);
            if (needed > 0)
            {
                text.resize(needed);
                MultiByteToWideChar(CP_ACP, 0, data.data(), static_cast<int>(len), &text[0], needed);
            }
            else
            {
                // As a last resort, replace non-decodable bytes with '?'
                for (size_t i = 0; i < len; ++i)
                    text.push_back((char)data[i] < 0 ? L'?' : static_cast<wchar_t>(data[i]));
            }
        }
    }

    // Update the edit control text and scroll to bottom
    SetWindowTextW(hEdit, text.c_str());
    // Move caret to end and scroll
    int textLen = (int)GetWindowTextLengthW(hEdit);
    SendMessageW(hEdit, EM_SETSEL, textLen, textLen);
    SendMessageW(hEdit, EM_SCROLLCARET, 0, 0);
}

void ShowLogWindow(HWND hParent)
{
    if (hLogWnd)
        return;

    RegisterLogWindowClass();

    hLogWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        L"PIT_LogWindow",
        L"PIT – Intune Fix (Log)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        300, 200, 720, 420,
        hParent, nullptr,
        GetModuleHandleW(nullptr),
        nullptr
    );

    // Create a Unicode-aware multiline read-only edit with vertical scrollbar.
    hEdit = CreateWindowW(
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL |
        ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_LEFT,
        10, 10, 690, 370,
        hLogWnd, nullptr, nullptr, nullptr
    );

    // Use default GUI font for better rendering (ClearType enabled by system).
    if (!hLogFont)
        hLogFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    SendMessageW(hEdit, WM_SETFONT, (WPARAM)hLogFont, TRUE);

    ShowWindow(hLogWnd, SW_SHOW);
    UpdateWindow(hLogWnd);

    SetTimer(hLogWnd, 1, 1000, nullptr);

    // Initial load
    LoadLog();
}


void UpdateLogWindow()
{
    if (hLogWnd && hEdit)
        LoadLog();
}

void CloseLogWindow()
{
    if (hLogWnd)
    {
        KillTimer(hLogWnd, 1);
        DestroyWindow(hLogWnd);
        hLogWnd = nullptr;
        hEdit = nullptr;
    }
}