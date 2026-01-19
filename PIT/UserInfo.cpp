
#include "UserInfo.h"
#include <windows.h>

std::wstring GetCurrentUserName()
{
    wchar_t buffer[256];
    DWORD size = static_cast<DWORD>(std::size(buffer));

    if (GetUserNameW(buffer, &size) && size > 1) {
        // size inclut le '\0' terminal → la chaîne utile fait (size-1)
        return std::wstring(buffer, size - 1);
    }

    // Secours : variable d'environnement (toujours locale)
    wchar_t envBuf[256];
    DWORD n = GetEnvironmentVariableW(L"USERNAME", envBuf, static_cast<DWORD>(std::size(envBuf)));
    if (n > 0 && n < std::size(envBuf)) {
        return std::wstring(envBuf, n);
    }

    return L"(Utilisateur inconnu)";
}

