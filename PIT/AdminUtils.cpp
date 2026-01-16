
#include "AdminUtils.h"
#include <windows.h>
#include <shellapi.h>

bool IsRunningAsAdmin()
{
    BOOL isAdmin = FALSE;
    HANDLE hToken = nullptr;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return false;

    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSID adminGroup = nullptr;

    if (AllocateAndInitializeSid(
        &ntAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &adminGroup))
    {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    CloseHandle(hToken);
    return isAdmin == TRUE;
}

bool RelaunchElevatedIfNeeded()
{
    if (IsRunningAsAdmin())
        return true;

    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    HINSTANCE res = ShellExecuteW(
        nullptr,
        L"runas",          // ⬅️ UAC
        exePath,
        nullptr,
        nullptr,
        SW_SHOWNORMAL
    );

    if ((INT_PTR)res <= 32)
        return false;

    return false; // on quitte l’instance actuelle
}
