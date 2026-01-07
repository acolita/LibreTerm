#include "CredentialManager.h"
#include <shlobj.h>
#include <sstream>

#pragma comment(lib, "shell32.lib")

std::wstring CredentialManager::GetConfigPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        std::wstring wpath(path);
        wpath += L"\\LibreTerm";
        CreateDirectory(wpath.c_str(), NULL);
        return wpath + L"\\credentials.ini";
    }
    return L"credentials.ini";
}

std::vector<Credential> CredentialManager::LoadCredentials() {
    std::vector<Credential> creds;
    std::wstring path = GetConfigPath();
    int count = GetPrivateProfileInt(L"Credentials", L"Count", 0, path.c_str());
    for (int i = 0; i < count; ++i) {
        std::wstringstream ss; ss << L"Cred_" << i;
        std::wstring section = ss.str();
        wchar_t buf[256];
        Credential c;
        GetPrivateProfileString(section.c_str(), L"Alias", L"", buf, 256, path.c_str()); c.alias = buf;
        if (c.alias.empty()) continue; 
        GetPrivateProfileString(section.c_str(), L"User", L"", buf, 256, path.c_str()); c.username = buf;
        GetPrivateProfileString(section.c_str(), L"Password", L"", buf, 256, path.c_str()); c.password = buf;
        GetPrivateProfileString(section.c_str(), L"KeyPath", L"", buf, 256, path.c_str()); c.keyPath = buf;
        creds.push_back(c);
    }
    return creds;
}

void CredentialManager::SaveCredentials(const std::vector<Credential>& creds) {
    std::wstring path = GetConfigPath();
    std::wstring countStr = std::to_wstring(creds.size());
    WritePrivateProfileString(L"Credentials", L"Count", countStr.c_str(), path.c_str());
    for (size_t i = 0; i < creds.size(); ++i) {
        std::wstringstream ss; ss << L"Cred_" << i;
        std::wstring section = ss.str();
        WritePrivateProfileString(section.c_str(), L"Alias", creds[i].alias.c_str(), path.c_str());
        WritePrivateProfileString(section.c_str(), L"User", creds[i].username.c_str(), path.c_str());
        WritePrivateProfileString(section.c_str(), L"Password", creds[i].password.c_str(), path.c_str());
        WritePrivateProfileString(section.c_str(), L"KeyPath", creds[i].keyPath.c_str(), path.c_str());
    }
}

Credential CredentialManager::GetCredential(const std::wstring& alias) {
    auto creds = LoadCredentials();
    for (const auto& c : creds) {
        if (c.alias == alias) return c;
    }
    return Credential();
}
