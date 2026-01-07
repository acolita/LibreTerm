#pragma once
#include <string>
#include <vector>
#include <windows.h>

struct Credential {
    std::wstring alias;
    std::wstring username;
    std::wstring password;
    std::wstring keyPath;
};

class CredentialManager {
public:
    static std::wstring GetConfigPath();
    static std::vector<Credential> LoadCredentials();
    static void SaveCredentials(const std::vector<Credential>& creds);
    static Credential GetCredential(const std::wstring& alias);
};
