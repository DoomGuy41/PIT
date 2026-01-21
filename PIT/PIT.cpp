
#include "framework.h"
#include "PIT.h"
#include "HPIA.h"
#include "PowerShellUtils.h"
#include "AdminUtils.h"
#include "GetHostname.h"
#include "UserInfo.h"
#include <shellapi.h>

#define MAX_LOADSTRING 100

// Boutons
#define BTN_HPIA            102
#define BTN_LANG_FR         103
#define BTN_GPUPDATE        104
#define BTN_NUMLOCK         105
#define BTN_PRINTER         106
#define BTN_REGEDIT         107
#define BTN_DEVMGMT         108
#define BTN_TASKMGR         109
#define BTN_INTUNE_FIX      110

// Globals
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

// UI globals
static HFONT  g_hFontMain = nullptr;
static HBRUSH g_hBrushWindow = nullptr;
static HBRUSH g_hBrushButton = nullptr;

// Forward declarations
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);




// ================================================================
// Entry point
// ================================================================
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    // 🔐 Admin check
    if (!IsRunningAsAdmin())
    {
        RelaunchElevatedIfNeeded();
        return 0;
    }

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PIT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}



// ================================================================
// Register class (THEME SOMBRE)
// ================================================================
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    static HBRUSH hDarkBg = CreateSolidBrush(RGB(32, 32, 32)); // 🔥 fond sombre propre

    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PIT));
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
    wcex.hbrBackground = hDarkBg;
    wcex.lpszClassName = szWindowClass;

    return RegisterClassExW(&wcex);
}



// ================================================================
// Create window (TAILLE FIXE + PAS DE RESIZE)
// ================================================================
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    // Taille CLIENT fixe
    const int CLIENT_W = 360;
    const int CLIENT_H = 560;

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    RECT rc = { 0, 0, CLIENT_W, CLIENT_H };
    AdjustWindowRect(&rc, style, FALSE);

    HWND hWnd = CreateWindowW(
        szWindowClass,
        szTitle,
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rc.right - rc.left,
        rc.bottom - rc.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hWnd)
        return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}



// ================================================================
// Window procedure
// ================================================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hLblHost = nullptr;
    static HWND hLblUser = nullptr;

    switch (message)
    {
        // ------------------------------------------------------------
        // CREATE UI
        // ------------------------------------------------------------

    case WM_CREATE:
    {
        // === Police ===
        LOGFONTW lf{};
        lf.lfHeight = -16;
        lf.lfWeight = FW_MEDIUM;
        wcscpy_s(lf.lfFaceName, L"Segoe UI");
        g_hFontMain = CreateFontIndirectW(&lf);

        g_hBrushButton = CreateSolidBrush(RGB(45, 45, 48));

        // === Hostname ===
        std::wstring host = L"Machine : " + GetHostName();
        HWND hLblHost = CreateWindowW(
            L"STATIC", host.c_str(),
            WS_CHILD | WS_VISIBLE,
            20, 12, 320, 22,
            hwnd, nullptr, nullptr, nullptr
        );
        SendMessageW(hLblHost, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);

        // === User ===
        std::wstring user = L"Utilisateur : " + GetCurrentUserName();
        HWND hLblUser = CreateWindowW(
            L"STATIC", user.c_str(),
            WS_CHILD | WS_VISIBLE,
            20, 36, 320, 20,
            hwnd, nullptr, nullptr, nullptr
        );
        SendMessageW(hLblUser, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);

        // === Boutons ===
        int x = 20;
        int y = 70;
        int w = 320;
        int h = 36;
        int gap = 6;

        HWND btn;

        btn = CreateWindowW(L"BUTTON", L"HPIA",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_HPIA, nullptr, nullptr);
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"Passer langue en français",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_LANG_FR, nullptr, nullptr);
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"GpUpdate + Mises à jour",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_GPUPDATE, nullptr, nullptr);
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"Activer Verr Num",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_NUMLOCK, nullptr, nullptr);
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"Portail imprimante",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_PRINTER, nullptr, nullptr);
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"Ouvrir Regedit",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_REGEDIT, nullptr, nullptr);
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"Gestionnaire périphériques",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_DEVMGMT, nullptr, nullptr);
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"Gestionnaire des tâches",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_TASKMGR, nullptr, nullptr);
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"Réparer applis Intune",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_INTUNE_FIX, nullptr, nullptr);
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);


        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        TrackMouseEvent(&tme);

    }
    break;


case WM_MOUSEMOVE:
    InvalidateRect(hwnd, nullptr, FALSE);
    break;


    // ------------------------------------------------------------
    // THEME SOMBRE (textes + boutons)
    // ------------------------------------------------------------
    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(220, 220, 220));
        SetBkMode(hdc, TRANSPARENT);
        return (INT_PTR)CreateSolidBrush(RGB(32, 32, 32));
    }

    case WM_CTLCOLORBTN:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(240, 240, 240));
        SetBkColor(hdc, RGB(45, 45, 48));
        return (INT_PTR)g_hBrushButton;
    }

    // ------------------------------------------------------------
    // BUTTON ACTIONS
    // ------------------------------------------------------------
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case BTN_HPIA:         RunHPIA(); break;
        case BTN_LANG_FR:      break;
        case BTN_GPUPDATE:     break;
        case BTN_NUMLOCK:      break;
        case BTN_PRINTER:      break;
        case BTN_REGEDIT:      ShellExecuteW(nullptr, L"open", L"regedit.exe", nullptr, nullptr, SW_SHOWNORMAL); break;
        case BTN_DEVMGMT:      ShellExecuteW(nullptr, L"open", L"devmgmt.msc", nullptr, nullptr, SW_SHOWNORMAL); break;
        case BTN_TASKMGR:      ShellExecuteW(nullptr, L"open", L"taskmgr.exe", nullptr, nullptr, SW_SHOWNORMAL); break;
        case BTN_INTUNE_FIX:   break;
        }
        break;

        // ------------------------------------------------------------
        // LOCK WINDOW SIZE
        // ------------------------------------------------------------
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 375;
        mmi->ptMinTrackSize.y = 520;
        mmi->ptMaxTrackSize.x = 375;
        mmi->ptMaxTrackSize.y = 520;
        return 0;
    }

    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;

        if (dis->CtlType != ODT_BUTTON)
            break;

        HDC hdc = dis->hDC;
        RECT rc = dis->rcItem;

        // États du bouton
        bool pressed = (dis->itemState & ODS_SELECTED);
        bool disabled = (dis->itemState & ODS_DISABLED);
        bool hot = (dis->itemState & ODS_HOTLIGHT);

        // Couleurs (thème sombre)

        COLORREF bgNormal = RGB(60, 63, 65);
        COLORREF bgHover = RGB(75, 78, 80);
        COLORREF bgPressed = RGB(45, 48, 50);
        COLORREF bgDisabled = RGB(90, 90, 90);

        COLORREF bgColor = bgNormal;
        if (disabled)      bgColor = bgDisabled;
        else if (pressed) bgColor = bgPressed;
        else if (hot)     bgColor = bgHover;  // ✅ effet hover


        // Fond arrondi
        HBRUSH hBrush = CreateSolidBrush(bgColor);
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(90, 90, 90));

        HGDIOBJ oldBrush = SelectObject(hdc, hBrush);
        HGDIOBJ oldPen = SelectObject(hdc, hPen);

        int radius = 28; // 🔥 rayon d’arrondi
        RoundRect(
            hdc,
            rc.left,
            rc.top,
            rc.right,
            rc.bottom,
            radius,
            radius
        );

        // Texte du bouton
        wchar_t text[128];
        GetWindowTextW(dis->hwndItem, text, 128);

        SetTextColor(hdc, RGB(230, 230, 230));
        SetBkMode(hdc, TRANSPARENT);

        DrawTextW(
            hdc,
            text,
            -1,
            &rc,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE
        );

        // Cleanup
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(hBrush);
        DeleteObject(hPen);

        return TRUE;
    }
    break;

    // ------------------------------------------------------------
    case WM_DESTROY:
        if (g_hFontMain) DeleteObject(g_hFontMain);
        if (g_hBrushButton) DeleteObject(g_hBrushButton);
        PostQuitMessage(0);
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}
