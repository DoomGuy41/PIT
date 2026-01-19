
#include "GetHostname.h"
#include <windows.h>

std::wstring GetHostName()
{
    // 1) Méthode locale standard : GetComputerNameW (NetBIOS local)
    //    Aucun accès réseau, aucun DNS, juste la valeur locale.
    wchar_t buffer[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;

    if (GetComputerNameW(buffer, &size))
    {
        return std::wstring(buffer, size); // size = nb de caractères (sans le '\0')
    }

    // 2) Secours : variable d'environnement (locale aussi)
    wchar_t envBuf[256];
    DWORD n = GetEnvironmentVariableW(L"COMPUTERNAME", envBuf, 256);
    if (n > 0 && n < 256)
    {
        return std::wstring(envBuf, n);
    }

    // 3) Échec
    return L"(Nom machine inconnu)";
}
