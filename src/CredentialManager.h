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
    static std::vector<Credential> LoadCredentials();
    static void SaveCredential(const Credential& cred);
    static void DeleteCredential(const std::wstring& alias);
    static Credential GetCredential(const std::wstring& alias);
};
