
#include "LogWindow.h"
#include <fstream>
#include <sstream>

static HWND hLogWnd = nullptr;
static HWND hEdit = nullptr;
static std::wstring g_LogPath = L"C:\\TEMP\\IntuneFix.log";


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

    hEdit = CreateWindowW(
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL |
        ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        10, 10, 690, 370,
        hLogWnd, nullptr, nullptr, nullptr
    );

    ShowWindow(hLogWnd, SW_SHOW);
    UpdateWindow(hLogWnd);

    SetTimer(hLogWnd, 1, 1000, nullptr);
}


static void LoadLog()
{
    std::wifstream file(g_LogPath);
    if (!file.good())
        return;

    std::wstringstream buffer;
    buffer << file.rdbuf();
    SetWindowTextW(hEdit, buffer.str().c_str());

    // Scroll en bas
    SendMessageW(hEdit, EM_LINESCROLL, 0, 99999);
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
