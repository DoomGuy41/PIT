#include <shlobj.h>
#include <fstream>
#include "PrinterShortcut.h"

// Helper interne (toujours static)
static std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    if (size == 0) return {};
    std::string s(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], size, nullptr, nullptr);
    return s;
}

// Fonction publique (SANS static)
bool CreateDesktopWebShortcut(const std::wstring& name, const std::wstring& url)
{
    PWSTR pPath = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Desktop, KF_FLAG_CREATE, nullptr, &pPath);
    if (FAILED(hr) || pPath == nullptr) return false;

    std::wstring desktop = pPath;
    CoTaskMemFree(pPath);

    if (desktop.empty()) return false;
    if (desktop.back() != L'\\') desktop.push_back(L'\\');

    std::wstring filename = name;
    // On s'assure que ça finit par .url
    if (filename.size() < 4 || _wcsicmp(filename.c_str() + filename.size() - 4, L".url") != 0)
        filename += L".url";

    std::wstring fullPath = desktop + filename;

    // --- CONSTRUCTION DU CONTENU DU FICHIER ---

    // 1. L'URL
    std::string content = "[InternetShortcut]\r\n";
    content += "URL=" + WideToUtf8(url) + "\r\n";

    // 2. L'ICONE
    // On utilise shell32.dll qui contient les icones système standard.
    // L'index 16 correspond à l'imprimante classique.
    content += "IconFile=C:\\Windows\\System32\\shell32.dll\r\n";
    content += "IconIndex=108\r\n";

    // ------------------------------------------

    std::ofstream ofs(fullPath, std::ios::binary);
    if (!ofs) return false;
    ofs.write(content.data(), (std::streamsize)content.size());
    ofs.close();
    return true;
}