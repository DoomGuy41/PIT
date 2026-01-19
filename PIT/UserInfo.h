
#pragma once
#include <string>

// Retourne le nom de l'utilisateur courant (local uniquement).
// Exemple: "mfekkar". En cas d'échec: "(Utilisateur inconnu)".
std::wstring GetCurrentUserName();
