
#include "VersionUtils.h"
#include <windows.h>
#include <vector>

#pragma comment(lib, "Version.lib")

std::wstring GetAppVersion()
{
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);

    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeW(path, &handle);
    if (size == 0)
        return L"";

    std::vector<BYTE> buffer(size);
    if (!GetFileVersionInfoW(path, handle, size, buffer.data()))
        return L"";

    VS_FIXEDFILEINFO* info = nullptr;
    UINT len = 0;

    if (!VerQueryValueW(
        buffer.data(),
        L"\\",
        reinterpret_cast<LPVOID*>(&info),
        &len))
        return L"";

    if (!info)
        return L"";

    WORD major = HIWORD(info->dwFileVersionMS);
    WORD minor = LOWORD(info->dwFileVersionMS);

    wchar_t version[64];
    swprintf_s(
        version,
        L"%d.%d",
        major, minor);

    return version;
}
