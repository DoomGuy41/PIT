#include "FranceLangPack.h"
#include "PowerShellUtils.h"

void frenchlangset()
{
    // Properly formed PowerShell commands separated by semicolons.
    // Run as a single -Command argument to powershell.exe.
    std::wstring cmd =
        L"Install-Language -Language fr-FR -Confirm:$false; "
        L"Set-SystemPreferredUILanguage -Language fr-FR; "
        L"Set-Culture fr-FR; "
        L"Set-WinSystemLocale fr-FR; "
        L"Set-WinUILanguageOverride fr-FR; "
        L"$lang = New-WinUserLanguageList fr-FR; Set-WinUserLanguageList -LanguageList $lang -Force; "
        L"Set-WinHomeLocation -GeoId 84; "
        L"control intl.cpl";

    RunPowerShellAsync(cmd);
}