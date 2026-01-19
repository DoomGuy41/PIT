
#pragma once
#include <string>

// Retourne le nom de la machine (local uniquement, NetBIOS).
// Jamais de DNS/AD, pas d'appel réseau.
// Exemple: "MONPC-1234".
// En cas d'échec, retourne "(Nom machine inconnu)".
std::wstring GetHostName();
