// PIT.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "PIT.h"
#include "HPIA.h"
#include "PowerShellUtils.h"
#include "AdminUtils.h"
#include "GetHostname.h"
#include "UserInfo.h"

#define MAX_LOADSTRING 100

#define BTN_HPIA            102
#define BTN_LANG_FR         103
#define BTN_GPUPDATE        104
#define BTN_NUMLOCK         105
#define BTN_PRINTER        106
#define BTN_REGEDIT        107
#define BTN_DEVMGMT        108
#define BTN_TASKMGR        109
#define BTN_INTUNE_FIX     110

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    
    {
        //  Vérification ADMIN AVANT TOUT
        if (!IsRunningAsAdmin())
        {
            RelaunchElevatedIfNeeded();
            return 0;
        }
    }

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PIT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PIT));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PIT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_PIT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}



LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    static HWND g_hHostnameLabel = nullptr;
    static HWND g_hUserLabel = nullptr;


    switch (message)
    {

    case WM_CREATE:
    {


        // Récupération du hostname
        std::wstring host = GetHostName();
        std::wstring labelText = L"Nom Machine : " + host;

        
        g_hHostnameLabel = CreateWindowExW(
            0, L"STATIC", labelText.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 10, 300, 24,  // x, y, w, h
            hwnd, nullptr, nullptr, nullptr
        );


        // 2) Utilisateur(ligne en - dessous)
            // Choisis l’une des deux fonctions selon ce que tu veux afficher :
            std::wstring user = GetCurrentUserName();        // local only
            //std::wstring user = GetCurrentUserNameSam();        // "Domaine\\Utilisateur"

        std::wstring userText = L"Utilisateur : " + user;

        g_hUserLabel = CreateWindowExW(
            0, L"STATIC", userText.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 10 + 24 + 4, 560, 22,   // y = 10 (hostname) + 24 (hauteur) + 4 (marge)
            hwnd, nullptr, nullptr, nullptr
        );


        // (Optionnel) mettre en gras
        LOGFONTW lf{};
        lf.lfHeight = -18;
        lf.lfWeight = FW_BOLD;
        wcscpy_s(lf.lfFaceName, L"Segoe UI");
        HFONT hFont = CreateFontIndirectW(&lf);
        SendMessageW(g_hHostnameLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

        int x = 20;
        int y = 20;
        int w = 300;
        int h = 36;
        int gap = 6;


        y += h + gap;
        CreateWindowW(L"BUTTON", L"HPIA seul",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, w, h,
            hwnd, (HMENU)BTN_HPIA, nullptr, nullptr);

        y += h + gap;
        CreateWindowW(L"BUTTON", L"Passer langue en français",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, w, h,
            hwnd, (HMENU)BTN_LANG_FR, nullptr, nullptr);

        y += h + gap;
        CreateWindowW(L"BUTTON", L"GpUpdate + Mises à jour",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, w, h,
            hwnd, (HMENU)BTN_GPUPDATE, nullptr, nullptr);

        y += h + gap;
        CreateWindowW(L"BUTTON", L"Activer Verr Num",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, w, h,
            hwnd, (HMENU)BTN_NUMLOCK, nullptr, nullptr);

        y += h + gap;
        CreateWindowW(L"BUTTON", L"Raccourci portail imprimante",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, w, h,
            hwnd, (HMENU)BTN_PRINTER, nullptr, nullptr);

        y += h + gap;
        CreateWindowW(L"BUTTON", L"Ouvrir Regedit",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, w, h,
            hwnd, (HMENU)BTN_REGEDIT, nullptr, nullptr);

        y += h + gap;
        CreateWindowW(L"BUTTON", L"Ouvrir Gestionnaire périphériques",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, w, h,
            hwnd, (HMENU)BTN_DEVMGMT, nullptr, nullptr);

        y += h + gap;
        CreateWindowW(L"BUTTON", L"Ouvrir Gestionnaire des tâches",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, w, h,
            hwnd, (HMENU)BTN_TASKMGR, nullptr, nullptr);

        y += h + gap;
        CreateWindowW(L"BUTTON", L"Réparer applis Intune en échec",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, w, h,
            hwnd, (HMENU)BTN_INTUNE_FIX, nullptr, nullptr);
    }
    break;



    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case BTN_HPIA:
            RunHPIA();
            break;

        case BTN_LANG_FR:
            // SetLanguageFR();
            break;

        case BTN_GPUPDATE:
            // RunGPUpdate();
            break;

        case BTN_NUMLOCK:
            // EnableNumLock();
            break;

        case BTN_PRINTER:
            // CreatePrinterShortcut();
            break;

        case BTN_REGEDIT:
            // OpenRegedit();
            break;

        case BTN_DEVMGMT:
            // OpenDeviceManager();
            break;

        case BTN_TASKMGR:
            // OpenTaskManager();
            break;

        case BTN_INTUNE_FIX:
           // RunIntuneFix();
            break;
        }
    }
    break;


    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return 0;
}


// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
