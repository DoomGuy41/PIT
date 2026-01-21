
#include "ToastNotification.h"
#include <windows.h>
#include <shellapi.h>
#include "Resource.h"

#define PIT_TRAY_UID 110

static NOTIFYICONDATAW g_nid{};
static bool g_trayInit = false;

static void InitTrayIcon(HWND hWnd)
{
    if (g_trayInit)
        return;

    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = hWnd;                 // ✅ OBLIGATOIRE
    g_nid.uID = PIT_TRAY_UID;
    g_nid.uFlags = NIF_ICON | NIF_TIP; // ✅ ICÔNE
    g_nid.hIcon = (HICON)LoadIconW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDI_PIT)
    );

    wcscpy_s(g_nid.szTip, L"PIT – Post Install Toolbox");

    Shell_NotifyIconW(NIM_ADD, &g_nid);
    g_trayInit = true;


    g_nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hWnd = hWnd;   // HWND de la fenêtre PIT

}

void ShowToast(
    HWND hWnd,
    const std::wstring& title,
    const std::wstring& message
)
{
    InitTrayIcon(hWnd);

    g_nid.uFlags = NIF_INFO;
    g_nid.dwInfoFlags = NIIF_INFO | NIIF_LARGE_ICON;

    wcsncpy_s(g_nid.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(g_nid.szInfo, message.c_str(), _TRUNCATE);

    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

void CleanupTray()
{
    if (g_trayInit)
    {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        g_trayInit = false;
    }
}
