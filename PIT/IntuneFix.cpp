
#include "PowerShellUtils.h"
#include <windows.h>
#include <fstream>
#include <string>
#include "resource.h"  // IDR_INTUNEFIX

// Écrit une ressource RCDATA sur disque
static bool WriteResourceToFile(WORD resId, const wchar_t* resType, const std::wstring& outPath)
{
    HMODULE hMod = GetModuleHandleW(nullptr);
    if (!hMod) return false;

    HRSRC hRes = FindResourceW(hMod, MAKEINTRESOURCEW(resId), resType);
    if (!hRes) return false;

    DWORD size = SizeofResource(hMod, hRes);
    if (!size) return false;

    HGLOBAL hData = LoadResource(hMod, hRes);
    if (!hData) return false;

    void* pData = LockResource(hData);
    if (!pData) return false;

    // Créer les dossiers C:\ProgramData\PIT\Scripts
    CreateDirectoryW(L"C:\\TEMP\\PIT", nullptr);
    CreateDirectoryW(L"C:\\TEMP\\PIT\\Scripts", nullptr);

    std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
    if (!out.good()) return false;

    out.write(static_cast<const char*>(pData), static_cast<std::streamsize>(size));
    out.close();
    return true;
}

void RunIntuneFix()
{
    const std::wstring scriptPath = L"C:\\TEMP\\PIT\\Scripts\\IntuneFix.ps1";

    // Déverse la ressource sur disque à chaque lancement (écrasement)
    if (!WriteResourceToFile(IDR_INTUNEFIX, RT_RCDATA, scriptPath))
    {
        MessageBoxW(nullptr, L"Échec d'extraction de la ressource IntuneFix.ps1",
            L"PIT - IntuneFix", MB_ICONERROR);
        return;
    }

    // IMPORTANT : ton utilitaire PowerShell construit déjà :
    //   powershell.exe -ExecutionPolicy Bypass -NoProfile -Command "<ici>"
    // Donc pour exécuter un fichier, on passe une commande d'invocation :
    //   & 'C:\ProgramData\PIT\Scripts\IntuneFix.ps1'
    std::wstring psCommand = L"& '" + scriptPath + L"'";

    RunPowerShellAsync(psCommand);
}
