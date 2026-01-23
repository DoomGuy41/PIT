#include "framework.h"
#include "PIT.h"
#include "HPIA.h"
#include "PowerShellUtils.h"
#include "AdminUtils.h"
#include "GetHostname.h"
#include "UserInfo.h"
#include <shellapi.h>
#include "IntuneFix.h"
#include "LogWindow.h"
#include <shobjidl.h>
#include "ToastNotification.h"
#include "VersionUtils.h"
#include <windowsx.h>
#include "PortailInstallerNative.h"
#include <commctrl.h>
#pragma comment(lib, "Comctl32.lib")
#include <sstream>
#include <iomanip>
#include <shlobj.h>
#include <fstream>
#include "PrinterShortcut.h"
#include "FranceLangPack.h"
#include "Regedit.h"


#define MAX_LOADSTRING 100

// Buttons
#define BTN_HPIA            102
#define BTN_LANG_FR         103
#define BTN_GPUPDATE        104
#define BTN_NUMLOCK         105
#define BTN_PRINTER         106
#define BTN_REGEDIT         107
#define BTN_DEVMGMT         108
#define BTN_TASKMGR         109
#define BTN_INTUNE_FIX      110
#define BTN_INSTALL_PORTAIL 111
#define BTN_ALL             112

// Messages from native installer (defined in PortailInstallerNative.h)
#ifndef WM_PORTAIL_PROGRESS
#define WM_PORTAIL_PROGRESS (WM_APP + 200)
#endif
#ifndef WM_PORTAIL_DONE
#define WM_PORTAIL_DONE     (WM_APP + 201)
#endif

// Timer to auto-hide the progress UI after completion
#define PORTAIL_HIDE_TIMER  (WM_APP + 202)
#define PORTAIL_HIDE_DELAY_MS 3000

// Globals
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

// UI globals
static HFONT  g_hFontMain = nullptr;
static HBRUSH g_hBrushWindow = nullptr;
static HBRUSH g_hBrushButton = nullptr;
static HWND   g_hProgressLabel = nullptr;
static HWND   g_hProgressBar = nullptr;

// DPI / mouse tracking / animation
static int    g_dpi = 96;
static bool   g_mouseTracking = false;

static const int BUTTON_COUNT = (BTN_ALL - BTN_HPIA + 1);
static HWND g_buttons[BUTTON_COUNT] = { nullptr };
static int  g_hoverTarget[BUTTON_COUNT] = { 0 }; // 0..255
static int  g_hoverAlpha[BUTTON_COUNT] = { 0 };  // 0..255

static HICON g_hIconBig = nullptr;
static HICON g_hIconSmall = nullptr;

static const UINT_PTR ANIM_TIMER_ID = 1001;
static const int ANIM_INTERVAL_MS = 16; // ~60fps

// Forward declarations
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

// Helpers
static inline int ButtonIndexFromId(int id) { return id - BTN_HPIA; }

static COLORREF BlendColor(COLORREF a, COLORREF b, int alpha)
{
    // alpha: 0..255 -> weight of b
    int inv = 255 - alpha;
    int ra = GetRValue(a), ga = GetGValue(a), ba = GetBValue(a);
    int rb = GetRValue(b), gb = GetGValue(b), bb = GetBValue(b);
    int r = (ra * inv + rb * alpha) / 255;
    int g = (ga * inv + gb * alpha) / 255;
    int bl = (ba * inv + bb * alpha) / 255;
    return RGB(r, g, bl);
}
// Entry point

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    //Admin check
    if (!IsRunningAsAdmin())
    {
        RelaunchElevatedIfNeeded();
        return 0;
    }
    // Prefer per-monitor v2 DPI awareness when available (best scaling).
    // Fall back to SetProcessDpiAwareness (shcore) or SetProcessDPIAware when needed.
    HMODULE hUser32 = LoadLibraryW(L"user32.dll");
    if (hUser32)
    {
        typedef BOOL(WINAPI* SetProcessDpiAwarenessContext_t)(HANDLE);
        SetProcessDpiAwarenessContext_t pSetDpiContext =
            (SetProcessDpiAwarenessContext_t)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");

        if (pSetDpiContext)
        {
            // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 == (DPI_AWARENESS_CONTEXT)-4
            pSetDpiContext((HANDLE)-4);
        }
        else
        {
            HMODULE hShcore = LoadLibraryW(L"shcore.dll");
            if (hShcore)
            {
                typedef HRESULT(WINAPI* SetProcessDpiAwareness_t)(int);
                SetProcessDpiAwareness_t pSetProcDpiAwareness =
                    (SetProcessDpiAwareness_t)GetProcAddress(hShcore, "SetProcessDpiAwareness");
                if (pSetProcDpiAwareness)
                {
                    // PROCESS_PER_MONITOR_DPI_AWARE = 2
                    pSetProcDpiAwareness(2);
                }
                FreeLibrary(hShcore);
            }
            else
            {
                typedef BOOL(WINAPI* SetProcessDPIAware_t)();
                SetProcessDPIAware_t pSetProcessDPIAware =
                    (SetProcessDPIAware_t)GetProcAddress(hUser32, "SetProcessDPIAware");
                if (pSetProcessDPIAware)
                    pSetProcessDPIAware();
            }
        }
        FreeLibrary(hUser32);
    }

    SetCurrentProcessExplicitAppUserModelID(
        L"PIT.PostInstall.Toolbox"
    );

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

// Register class (Dark Theme)

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    static HBRUSH hDarkBg = CreateSolidBrush(RGB(32, 32, 32)); // background brush

    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    // We'll set window icons in WM_CREATE with DPI-aware sizes.
    wcex.hIcon = nullptr;
    wcex.hIconSm = nullptr;
    wcex.hbrBackground = hDarkBg;
    wcex.lpszClassName = szWindowClass;

    return RegisterClassExW(&wcex);
}

// Create window (fixed size w/ resize)
// - now scales client size according to system DPI and button count
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    // Base client width (design time at 96 DPI)
    const int CLIENT_W = 360;

    // Button layout baseline (design-time values at 96 DPI)
    const int BASE_TOP_OFFSET = 70;
    const int BASE_BUTTON_H = 36;
    const int BASE_GAP = 6;
    const int BASE_BOTTOM_MARGIN = 120; // space for version label / margins

    // Compute how many buttons we have (uses defines above)
    const int logicalButtonCount = BUTTON_COUNT; // BTN_ALL - BTN_HPIA + 1

    // Compute client height in logical pixels (96 DPI baseline)
    int logicalClientH = BASE_TOP_OFFSET
        + logicalButtonCount * BASE_BUTTON_H
        + (logicalButtonCount - 1) * BASE_GAP
        + BASE_BOTTOM_MARGIN;

    // Query system DPI to scale the initial window size
    HDC screen = GetDC(nullptr);
    int sysDpi = GetDeviceCaps(screen, LOGPIXELSY);
    ReleaseDC(nullptr, screen);
    if (sysDpi > 0)
        g_dpi = sysDpi;

    int scaledW = MulDiv(CLIENT_W, g_dpi, 96);
    int scaledH = MulDiv(logicalClientH, g_dpi, 96);

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    RECT rc = { 0, 0, scaledW, scaledH };
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



// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hLblHost = nullptr;
    static HWND hLblUser = nullptr;

    switch (message)
    {
        // Progress messages from native installer
    case WM_PORTAIL_PROGRESS:
    {
        int index = (int)wParam;
        int percent = (int)lParam;
        if (g_hProgressLabel == nullptr || g_hProgressBar == nullptr)
            break;

        std::wostringstream ss;
        ss << L"Installing package #" << index << L" — " << percent << L"%";
        SetWindowTextW(g_hProgressLabel, ss.str().c_str());
        SendMessageW(g_hProgressBar, PBM_SETPOS, percent, 0);
        if (!IsWindowVisible(g_hProgressLabel))
        {
            ShowWindow(g_hProgressLabel, SW_SHOW);
            ShowWindow(g_hProgressBar, SW_SHOW);
        }
        return 0;
    }

    case WM_PORTAIL_DONE:
    {
        int index = (int)wParam;
        HRESULT hr = (HRESULT)lParam;
        if (g_hProgressLabel && g_hProgressBar)
        {
            if (hr == S_OK || hr == 0)
            {
                std::wostringstream ss;
                ss << L"Package #" << index << L" installed.";
                SetWindowTextW(g_hProgressLabel, ss.str().c_str());
                SendMessageW(g_hProgressBar, PBM_SETPOS, 100, 0);
                ShowToast(hwnd, L"Portail – Succès", ss.str());
            }
            else
            {
                std::wostringstream ss;
                ss << L"Package #" << index << L" failed (0x" << std::hex << hr << L")";
                SetWindowTextW(g_hProgressLabel, ss.str().c_str());
                SendMessageW(g_hProgressBar, PBM_SETPOS, 0, 0);
                ShowToast(hwnd, L"Portail – Erreur", ss.str());
            }
            // auto-hide after a short delay
            SetTimer(hwnd, PORTAIL_HIDE_TIMER, PORTAIL_HIDE_DELAY_MS, nullptr);
        }
        return 0;
    }

    // CREATE UI
    case WM_CREATE:
    {
        // Recompute DPI for this window (per-monitor aware if available)
        HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
        if (hUser32)
        {
            typedef UINT(WINAPI* GetDpiForWindow_t)(HWND);
            GetDpiForWindow_t pGetDpiForWindow = (GetDpiForWindow_t)GetProcAddress(hUser32, "GetDpiForWindow");
            if (pGetDpiForWindow)
                g_dpi = (int)pGetDpiForWindow(hwnd);
        }

        // initialize progress common control class
        INITCOMMONCONTROLSEX icex{};
        icex.dwSize = sizeof(icex);
        icex.dwICC = ICC_PROGRESS_CLASS;
        InitCommonControlsEx(&icex);

        // === Font with ClearType smoothing ===
        LOGFONTW lf{};
        lf.lfHeight = -MulDiv(16, g_dpi, 96);
        lf.lfWeight = FW_MEDIUM;
        lf.lfQuality = CLEARTYPE_NATURAL_QUALITY;
        wcscpy_s(lf.lfFaceName, L"Segoe UI");
        g_hFontMain = CreateFontIndirectW(&lf);

        g_hBrushButton = CreateSolidBrush(RGB(45, 45, 48));
        g_hBrushWindow = CreateSolidBrush(RGB(32, 32, 32)); // reuse in WM_CTLCOLORSTATIC

        // compute scaled metrics
        auto S = [](int value) { return MulDiv(value, g_dpi, 96); };

        // === Hostname ===
        std::wstring host = L"Machine : " + GetHostName();
        hLblHost = CreateWindowW(
            L"STATIC", host.c_str(),
            WS_CHILD | WS_VISIBLE,
            S(20), S(12), S(320), S(22),
            hwnd, nullptr, nullptr, nullptr
        );
        SendMessageW(hLblHost, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);

        // === User ===
        std::wstring user = L"Utilisateur : " + GetCurrentUserName();
        hLblUser = CreateWindowW(
            L"STATIC", user.c_str(),
            WS_CHILD | WS_VISIBLE,
            S(20), S(36), S(320), S(20),
            hwnd, nullptr, nullptr, nullptr
        );
        SendMessageW(hLblUser, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);


        std::wstring version = L"Version " + GetAppVersion();

        HWND hLblVersion = CreateWindowW(
            L"STATIC",
            version.c_str(),
            WS_CHILD | WS_VISIBLE | SS_RIGHT,
            S(-115), S(535), S(200), S(50),
            hwnd,
            nullptr,
            nullptr,
            nullptr
        );
        SendMessageW(hLblVersion, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);



        // === Boutons (store handles in g_buttons array) ===
        int x = S(20);
        int y = S(70);
        int w = S(320);
        int h = S(36);
        int gap = S(6);

        HWND btn;
        int bi = 0;

        btn = CreateWindowW(L"BUTTON", L"HPIA",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_HPIA, nullptr, nullptr);
        g_buttons[bi++] = btn;
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"Langue français os / Office",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_LANG_FR, nullptr, nullptr);
        g_buttons[bi++] = btn;
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"Sync / Mise a jours",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_GPUPDATE, nullptr, nullptr);
        g_buttons[bi++] = btn;
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"Activer Verr Num",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_NUMLOCK, nullptr, nullptr);
        g_buttons[bi++] = btn;
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"Portail imprimante",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_PRINTER, nullptr, nullptr);
        g_buttons[bi++] = btn;
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"Ouvrir Regedit",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_REGEDIT, nullptr, nullptr);
        g_buttons[bi++] = btn;
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"Gestionnaire périphériques",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_DEVMGMT, nullptr, nullptr);
        g_buttons[bi++] = btn;
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"Gestionnaire des tâches",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_TASKMGR, nullptr, nullptr);
        g_buttons[bi++] = btn;
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"Réparer applis Intune",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_INTUNE_FIX, nullptr, nullptr);
        g_buttons[bi++] = btn;
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        // New button: Installer Portail Entreprise
        btn = CreateWindowW(L"BUTTON", L"Installer portail entreprise",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_INSTALL_PORTAIL, nullptr, nullptr);
        g_buttons[bi++] = btn;
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        y += h + gap;

        btn = CreateWindowW(L"BUTTON", L"Tout",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, w, h, hwnd, (HMENU)BTN_ALL, nullptr, nullptr);
        g_buttons[bi++] = btn;
        SendMessageW(btn, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);


        // load DPI-scaled icons and set them on the window
        int bigSize = MulDiv(48, g_dpi, 96);
        int smallSize = MulDiv(16, g_dpi, 96);

        if (g_hIconBig) { DestroyIcon(g_hIconBig); g_hIconBig = nullptr; }
        if (g_hIconSmall) { DestroyIcon(g_hIconSmall); g_hIconSmall = nullptr; }

        g_hIconBig = (HICON)LoadImageW(hInst, MAKEINTRESOURCE(IDI_PIT), IMAGE_ICON, bigSize, bigSize, LR_DEFAULTCOLOR);
        g_hIconSmall = (HICON)LoadImageW(hInst, MAKEINTRESOURCE(IDI_PIT), IMAGE_ICON, smallSize, smallSize, LR_DEFAULTCOLOR);

        if (g_hIconBig) SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_hIconBig);
        if (g_hIconSmall) SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSmall);

        // determine positions for progress UI based on client rect
        RECT rcClient{};
        GetClientRect(hwnd, &rcClient);
        int clientH = rcClient.bottom;
        int lblY = clientH - S(84);
        int barY = lblY + S(22);

        // create progress label + progress bar (hidden initially)
        g_hProgressLabel = CreateWindowW(L"STATIC", L"", WS_CHILD /*hidden*/,
            x, lblY, w, S(18), hwnd, nullptr, nullptr, nullptr);
        SendMessageW(g_hProgressLabel, WM_SETFONT, (WPARAM)g_hFontMain, TRUE);
        ShowWindow(g_hProgressLabel, SW_HIDE);

        g_hProgressBar = CreateWindowExW(0, PROGRESS_CLASS, nullptr,
            WS_CHILD | PBS_SMOOTH /*hidden*/,
            x, barY, w, S(12), hwnd, nullptr, nullptr, nullptr);
        SendMessageW(g_hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessageW(g_hProgressBar, PBM_SETPOS, 0, 0);
        ShowWindow(g_hProgressBar, SW_HIDE);

        // initialize tracking state (will be armed on first WM_MOUSEMOVE)
        g_mouseTracking = false;

        // start animation timer
        SetTimer(hwnd, ANIM_TIMER_ID, ANIM_INTERVAL_MS, nullptr);
    }
    break;

    case WM_MOUSEMOVE:
    {
        // Ensure we receive WM_MOUSELEAVE once the cursor exits the window
        if (!g_mouseTracking)
        {
            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            g_mouseTracking = true;
        }

        // Which child is under cursor?
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        HWND child = ChildWindowFromPointEx(hwnd, pt, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);
        int hoveredIndex = -1;
        if (child && child != hwnd)
        {
            int id = GetDlgCtrlID(child);
            if (id >= BTN_HPIA && id <= BTN_ALL)
                hoveredIndex = ButtonIndexFromId(id);
        }

        for (int i = 0; i < BUTTON_COUNT; ++i)
            g_hoverTarget[i] = (i == hoveredIndex) ? 255 : 0;

        // Parent invalidation so buttons pick up owner-draw state
        InvalidateRect(hwnd, nullptr, FALSE);
    }
    break;

    case WM_MOUSELEAVE:
    {
        g_mouseTracking = false;
        for (int i = 0; i < BUTTON_COUNT; ++i)
            g_hoverTarget[i] = 0;
        InvalidateRect(hwnd, nullptr, FALSE);
    }
    break;

    // ------------------------------------------------------------
    // THEME SOMBRE (textes + boutons)
    // ------------------------------------------------------------
    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(220, 220, 220));
        SetBkMode(hdc, TRANSPARENT);
        return (INT_PTR)g_hBrushWindow;
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
        case BTN_ALL:     
            InstallPortailPackagesNativeAsync(hwnd);
            RunIntuneFix();
            ShowLogWindow(hwnd);
            CreateDesktopWebShortcut(shortcutName, portalUrl);
            SetInitialKeyboardIndicators();
            ShellExecuteW(NULL, L"runas", L"gpupdate.exe", L"/force", NULL, SW_HIDE);
            ShellExecuteW(NULL, L"open", L"USOClient.exe", L"StartInteractiveScan", NULL, SW_HIDE);
            ShellExecuteW(NULL, L"open", L"deviceenroller.exe", L"/c /mobiledevice", NULL, SW_HIDE);
            ShellExecuteW(NULL, L"open", L"ms-settings:workplace", NULL, NULL, SW_SHOWNORMAL);
            frenchlangset();
            ShellExecuteW(NULL, L"runas", L"C:\\Temp\\PIT\\OfficeSetup.exe", NULL, NULL, SW_SHOWNORMAL);
            RunHPIA();
            ShowToast(
                hwnd,
                L"Post Install Toolbox",
				L"Toutes les actions ont été lancées en arrière-plan.");
            break;

        case BTN_HPIA:
            RunHPIA();
            ShowToast(
                hwnd,
                L"Post Install Toolbox  – HPIA",
                L"HPIA a été lancé en arrière-plan.");
            break;

        case BTN_LANG_FR:  
            frenchlangset();
            ShellExecuteW(NULL, L"runas", L"C:\\Temp\\PIT\\OfficeSetup.exe", NULL, NULL, SW_SHOWNORMAL);
            break;

        case BTN_GPUPDATE:    
        {
            //  Force la mise à jour des stratégies (GPO) en arrière-plan
            ShellExecuteW(NULL, L"runas", L"gpupdate.exe", L"/force", NULL, SW_HIDE);

            //  Lance le scan des mises à jour via USOClient
            ShellExecuteW(NULL, L"open", L"USOClient.exe", L"StartInteractiveScan", NULL, SW_HIDE);

            //  Déclenche la synchro système
            ShellExecuteW(NULL, L"open", L"deviceenroller.exe", L"/c /mobiledevice", NULL, SW_HIDE);

            ShellExecuteW(NULL,L"open", L"ms-settings:workplace", NULL, NULL, SW_SHOWNORMAL);

            //. Informe l'utilisateur
            ShowToast(hwnd, L"Portail d'entreprise", L"Synchronisation lancée en arrière-plan...");

            ShowToast(hwnd, L"Windows Update", L"Recherche de mises à jour lancée...");
        }
        break;

        case BTN_NUMLOCK: {
            if (SetInitialKeyboardIndicators()) {
                ShowToast(hwnd, L"Configuration", L"NumLock activé pour le démarrage.");
            }
            else {
                ShowToast(hwnd, L"Erreur", L"Échec de la modification (Droits Admin requis).");
            }
        }
         break;
        case BTN_PRINTER:
        {
            bool ok = CreateDesktopWebShortcut(shortcutName, portalUrl);
            if (ok)
                ShowToast(hwnd, L"Portail", L"Raccourci créé sur le Bureau.");
            else
                ShowToast(hwnd, L"Portail", L"Impossible de créer le raccourci sur le Bureau.");
        }
        break;

        case BTN_REGEDIT:
            ShellExecuteW(nullptr, L"open", L"regedit.exe", nullptr, nullptr, SW_SHOWNORMAL);
            break;

        case BTN_DEVMGMT:
            ShellExecuteW(nullptr, L"open", L"devmgmt.msc", nullptr, nullptr, SW_SHOWNORMAL);
            break;

        case BTN_TASKMGR:
            ShellExecuteW(nullptr, L"open", L"taskmgr.exe", nullptr, nullptr, SW_SHOWNORMAL);
            break;

        case BTN_INTUNE_FIX:
            RunIntuneFix();
            ShowLogWindow(hwnd);
            ShowToast(
                hwnd,
                L"Post Install Toolbox – Intune",
                L"Le correctif Intune a été lancé en arrière-plan.");
            break;

        case BTN_INSTALL_PORTAIL:
            // start native installer (async). UI receives WM_PORTAIL_PROGRESS/WM_PORTAIL_DONE.
            InstallPortailPackagesNativeAsync(hwnd);
            ShowToast(hwnd, L"Post Install Toolbox – Portail", L"Installation du portail entreprise lancée en arrière-plan.");
            break;
        }
        break;

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        // scale fixed window dimensions according to DPI set at WM_CREATE / InitInstance
        mmi->ptMinTrackSize.x = MulDiv(375, g_dpi, 96);
        mmi->ptMinTrackSize.y = MulDiv(595, g_dpi, 96);
        mmi->ptMaxTrackSize.x = MulDiv(375, g_dpi, 96);
        mmi->ptMaxTrackSize.y = MulDiv(595, g_dpi, 96);
        return 0;
    }

    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;

        if (dis->CtlType != ODT_BUTTON)
            break;

        HDC hdc = dis->hDC;
        RECT rc = dis->rcItem;

        // Determine which button index this is
        int idx = -1;
        for (int i = 0; i < BUTTON_COUNT; ++i)
        {
            if (g_buttons[i] == dis->hwndItem) { idx = i; break; }
        }

        // States
        bool pressed = (dis->itemState & ODS_SELECTED);
        bool disabled = (dis->itemState & ODS_DISABLED);
        int alpha = (idx >= 0) ? g_hoverAlpha[idx] : 0;

        // Colors (dark theme)
        COLORREF bgNormal = RGB(60, 63, 65);
        COLORREF bgHover = RGB(75, 78, 80);
        COLORREF bgPressed = RGB(45, 48, 50);
        COLORREF bgDisabled = RGB(90, 90, 90);

        COLORREF bgColor = bgNormal;
        if (disabled) bgColor = bgDisabled;
        else if (pressed) bgColor = bgPressed;
        else if (alpha > 0) bgColor = BlendColor(bgNormal, bgHover, alpha);

        // Rounded background
        HBRUSH hBrush = CreateSolidBrush(bgColor);
        HPEN hPen = CreatePen(PS_SOLID, MulDiv(1, g_dpi, 96), RGB(90, 90, 90));

        HGDIOBJ oldBrush = SelectObject(hdc, hBrush);
        HGDIOBJ oldPen = SelectObject(hdc, hPen);

        int radius = MulDiv(28, g_dpi, 96); // scale radius with DPI
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);

        // Text
        wchar_t text[128];
        GetWindowTextW(dis->hwndItem, text, 128);

        SetTextColor(hdc, RGB(230, 230, 230));
        SetBkMode(hdc, TRANSPARENT);

        DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Cleanup
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(hBrush);
        DeleteObject(hPen);

        return TRUE;
    }
    break;

    case WM_TIMER:
        if (wParam == ANIM_TIMER_ID)
        {
            bool needInvalidate = false;

            // easing factor: proportion of the remaining distance to move each tick
            const float easeFactor = 0.22f; // smaller -> slower, larger -> snappier

            for (int i = 0; i < BUTTON_COUNT; ++i)
            {
                int cur = g_hoverAlpha[i];
                int tgt = g_hoverTarget[i];
                if (cur == tgt) continue;

                // smooth easing toward target
                float nextF = cur + (tgt - cur) * easeFactor;
                int next = (int)(nextF + (nextF > cur ? 0.5f : -0.5f));

                // ensure progress and final snap to target
                if (abs(next - cur) < 1)
                    next = tgt;

                g_hoverAlpha[i] = next;

                if (g_buttons[i])
                    InvalidateRect(g_buttons[i], nullptr, FALSE);

                needInvalidate = true;
            }

            if (needInvalidate)
                UpdateWindow(hwnd);
        }
        else if (wParam == PORTAIL_HIDE_TIMER)
        {
            // hide progress UI after a short delay
            if (g_hProgressLabel) ShowWindow(g_hProgressLabel, SW_HIDE);
            if (g_hProgressBar) ShowWindow(g_hProgressBar, SW_HIDE);
            KillTimer(hwnd, PORTAIL_HIDE_TIMER);
        }
        else
        {
            UpdateLogWindow();
        }
        break;

    case WM_DESTROY:
        KillTimer(hwnd, ANIM_TIMER_ID);
        if (g_hFontMain) DeleteObject(g_hFontMain);
        if (g_hBrushButton) DeleteObject(g_hBrushButton);
        if (g_hBrushWindow) DeleteObject(g_hBrushWindow);
        if (g_hIconBig) { DestroyIcon(g_hIconBig); g_hIconBig = nullptr; }
        if (g_hIconSmall) { DestroyIcon(g_hIconSmall); g_hIconSmall = nullptr; }
        CleanupTray();
        PostQuitMessage(0);
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}