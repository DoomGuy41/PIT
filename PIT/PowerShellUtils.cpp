
#include "PowerShellUtils.h"

#include <windows.h>
#include <string>
#include <vector>

/// Exécute une commande PowerShell (bloquant)
void RunPowerShell(const std::wstring& command)
{
    // Construction de la ligne de commande
    std::wstring fullCmd =
        L"powershell.exe -ExecutionPolicy Bypass -NoProfile -Command \"" +
        command + L"\"";

    // CreateProcessW exige un buffer modifiable
    std::vector<wchar_t> cmdLine(fullCmd.begin(), fullCmd.end());
    cmdLine.push_back(L'\0'); // null termination

    STARTUPINFOW si{};
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi{};

    BOOL ok = CreateProcessW(
        nullptr,                  // Application name
        cmdLine.data(),           // Ligne de commande MODIFIABLE
        nullptr,                  // Process security
        nullptr,                  // Thread security
        FALSE,                    // Inherit handles
        CREATE_NO_WINDOW,         // Flags
        nullptr,                  // Environment
        nullptr,                  // Current directory
        &si,                      // Startup info
        &pi                       // Process info
    );

    if (!ok)
    {
        MessageBoxW(
            nullptr,
            L"Impossible de lancer PowerShell.",
            L"PIT - Erreur",
            MB_ICONERROR
        );
        return;
    }

    // Attendre la fin du process
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Nettoyage
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

/// Thread wrapper
static DWORD WINAPI PowerShellThread(LPVOID param)
{
    std::wstring* cmd = static_cast<std::wstring*>(param);
    RunPowerShell(*cmd);
    delete cmd;
    return 0;
}

/// Exécute une commande PowerShell en arrière-plan (non bloquant)
void RunPowerShellAsync(const std::wstring& command)
{
    std::wstring* cmd = new std::wstring(command);

    HANDLE hThread = CreateThread(
        nullptr,
        0,
        PowerShellThread,
        cmd,
        0,
        nullptr
    );

    if (hThread)
        CloseHandle(hThread);
}
