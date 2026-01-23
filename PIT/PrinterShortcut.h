#pragma once
#include <string>

const std::wstring shortcutName = L"Portail Imprimante";
const std::wstring portalUrl = L"https://print.corp.skf.net/portal"; // <- change to real URL
bool CreateDesktopWebShortcut(const std::wstring& name, const std::wstring& url);