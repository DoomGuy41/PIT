#pragma once
#include <windows.h>

#define WM_PORTAIL_PROGRESS (WM_APP + 200)
#define WM_PORTAIL_DONE     (WM_APP + 201)

// Starts installation of all .appx/.msix/.appxbundle packages found in C:\TEMP\PIT\PortailPackages.
// Runs on a worker thread. Progress updates are posted to the provided hwnd using WM_PORTAIL_PROGRESS:
//   wParam = package index (0..N-1), lParam = percent (0..100).
// When a package completes, WM_PORTAIL_DONE is posted:
//   wParam = package index, lParam = HRESULT (0 == success).
void InstallPortailPackagesNativeAsync(HWND hwnd);
