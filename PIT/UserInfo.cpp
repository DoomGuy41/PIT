
#include "UserInfo.h"
#include <windows.h>
#include <Wtsapi32.h>
#include <string>

#pragma comment(lib, "Wtsapi32.lib")

std::wstring GetCurrentUserName()
{
    // 1️⃣ Utilisateur connecté à la session console (fiable)
    DWORD sessionId = WTSGetActiveConsoleSessionId();

    LPWSTR userName = nullptr;
    DWORD length = 0;

    if (WTSQuerySessionInformationW(
        WTS_CURRENT_SERVER_HANDLE,
        sessionId,
        WTSUserName,
        &userName,
        &length))
    {
        if (userName && wcslen(userName) > 0)
        {
            std::wstring result(userName);
            WTSFreeMemory(userName);
            return result;
        }
        WTSFreeMemory(userName);
    }

    // 2️⃣ Fallback : token courant (cas très rare)
    wchar_t buffer[256];
    DWORD size = static_cast<DWORD>(std::size(buffer));

    if (GetUserNameW(buffer, &size) && size > 1)
    {
        return std::wstring(buffer, size - 1);
    }

    // 3️⃣ Dernier secours : variable d’environnement
    wchar_t envBuf[256];
    DWORD n = GetEnvironmentVariableW(
        L"USERNAME",
        envBuf,
        static_cast<DWORD>(std::size(envBuf)));

    if (n > 0 && n < std::size(envBuf))
    {
        return std::wstring(envBuf, n);
    }

    return L"(Utilisateur inconnu)";
}
