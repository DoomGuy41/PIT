#include <windows.h>
#include <iostream>
#include <string>
#include "Regedit.h"

bool SetInitialKeyboardIndicators()
{
    HKEY hKey;
    // Chemin de la clé : .DEFAULT\Control Panel\Keyboard
    LPCWSTR subkey = L".DEFAULT\\Control Panel\\Keyboard";
    LPCWSTR valueName = L"InitialKeyboardIndicators";
    LPCWSTR data = L"2"; // La valeur souhaitée

    // 1. Ouvrir la clé de registre
    // HKEY_USERS correspond au lecteur HKU: de PowerShell
    LSTATUS status = RegOpenKeyExW(HKEY_USERS, subkey, 0, KEY_SET_VALUE, &hKey);

    if (status != ERROR_SUCCESS) {
        // Probablement un problème de droits (nécessite d'être Admin)
        return false;
    }

    // 2. Fixer la valeur (REG_SZ car c'est une chaîne de caractères dans ce cas précis)
    status = RegSetValueExW(
        hKey,
        valueName,
        0,
        REG_SZ,
        (const BYTE*)data,
        (DWORD)((wcslen(data) + 1) * sizeof(wchar_t))
    );

    // 3. Fermer la clé
    RegCloseKey(hKey);

    return (status == ERROR_SUCCESS);
}