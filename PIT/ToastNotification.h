#pragma once
#include <string>
#include <windows.h>

#define WM_TRAYICON (WM_APP + 100)

void ShowToast(
    HWND hWnd,
    const std::wstring& title,
    const std::wstring& message
);

void CleanupTray();
