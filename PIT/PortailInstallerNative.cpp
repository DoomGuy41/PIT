#include "PortailInstallerNative.h"

#include <thread>
#include <filesystem>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Management.Deployment.h>

#pragma comment(lib, "WindowsApp.lib")

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Management::Deployment;
namespace fs = std::filesystem;

static std::wstring ToFileUri(const std::wstring& path)
{
    // Convert "C:\foo\bar.appx" -> "file:///C:/foo/bar.appx"
    std::wstring uri = L"file:///";
    for (wchar_t c : path)
    {
        if (c == L'\\') uri.push_back(L'/');
        else uri.push_back(c);
    }
    return uri;
}

void InstallPortailPackagesNativeAsync(HWND hwnd)
{
    std::thread([hwnd]() {
        init_apartment();

        const std::wstring pkgDir = L"C:\\TEMP\\PIT\\PortailPackages";
        std::vector<std::wstring> packages;

        try
        {
            if (!fs::exists(pkgDir) || !fs::is_directory(pkgDir))
            {
                PostMessageW(hwnd, WM_PORTAIL_DONE, static_cast<WPARAM>(0), static_cast<LPARAM>(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)));
                return;
            }

            for (auto& entry : fs::directory_iterator(pkgDir))
            {
                if (!entry.is_regular_file()) continue;
                auto ext = entry.path().extension().wstring();
                if (_wcsicmp(ext.c_str(), L".appx") == 0 ||
                    _wcsicmp(ext.c_str(), L".msix") == 0 ||
                    _wcsicmp(ext.c_str(), L".appxbundle") == 0)
                {
                    packages.push_back(entry.path().wstring());
                }
            }

            if (packages.empty())
            {
                PostMessageW(hwnd, WM_PORTAIL_DONE, static_cast<WPARAM>(0), static_cast<LPARAM>(HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES)));
                return;
            }
        }
        catch (...)
        {
            PostMessageW(hwnd, WM_PORTAIL_DONE, static_cast<WPARAM>(0), static_cast<LPARAM>(E_FAIL));
            return;
        }

        PackageManager pm;

        for (size_t i = 0; i < packages.size(); ++i)
        {
            const std::wstring& pkgPath = packages[i];
            std::wstring uriStr = ToFileUri(pkgPath);

            try
            {
                Uri pkgUri{ winrt::hstring(uriStr) };

                // empty dependency list (AddPackageAsync requires an iterable param)
                auto depUris = single_threaded_vector<Uri>();

                // Start async install operation
                auto op = pm.AddPackageAsync(pkgUri, depUris, DeploymentOptions::None);

                // Wire progress handler -> post percentage to UI
                op.Progress([hwnd, idx = static_cast<int>(i)](IAsyncOperationWithProgress<DeploymentResult, DeploymentProgress> const&, DeploymentProgress const& progress)
                    {
                        int percent = progress.percentage;
                        if (percent < 0) percent = 0;
                        if (percent > 100) percent = 100;
                        PostMessageW(hwnd, WM_PORTAIL_PROGRESS, static_cast<WPARAM>(idx), static_cast<LPARAM>(percent));
                    });

                // Wait for completion (we're on a worker thread)
                DeploymentResult result = op.get();

                // DeploymentResult exposes ExtendedErrorCode() for HRESULT-style error.
                HRESULT hr = result.ExtendedErrorCode();

                // Post final completion for this package (0 => success)
                PostMessageW(hwnd, WM_PORTAIL_DONE, static_cast<WPARAM>(i), static_cast<LPARAM>(hr));

                std::this_thread::sleep_for(std::chrono::milliseconds(150));
            }
            catch (hresult_error const& ex)
            {
                PostMessageW(hwnd, WM_PORTAIL_DONE, static_cast<WPARAM>(i), static_cast<LPARAM>(ex.code()));
            }
            catch (...)
            {
                PostMessageW(hwnd, WM_PORTAIL_DONE, static_cast<WPARAM>(i), static_cast<LPARAM>(E_FAIL));
            }
        }
        }).detach();
}