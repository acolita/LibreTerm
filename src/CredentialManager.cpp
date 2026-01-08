#include "CredentialManager.h"
#include <windows.h>
#include <wincred.h>
#include <sstream>

#pragma comment(lib, "Advapi32.lib")

static const std::wstring PREFIX = L"LibreTerm:";

std::vector<Credential> CredentialManager::LoadCredentials() {
    std::vector<Credential> creds;
    DWORD count = 0;
    PCREDENTIALW* pCreds = NULL;

    // Enumerate generic credentials starting with our prefix
    if (CredEnumerateW(NULL, 0, &count, &pCreds)) {
        for (DWORD i = 0; i < count; ++i) {
            std::wstring target = pCreds[i]->TargetName;
            if (target.find(PREFIX) == 0) {
                Credential c;
                c.alias = target.substr(PREFIX.length());
                c.username = pCreds[i]->UserName ? pCreds[i]->UserName : L"";
                
                // Blob contains: Password + | + KeyPath
                if (pCreds[i]->CredentialBlobSize > 0) {
                    std::wstring blob((wchar_t*)pCreds[i]->CredentialBlob, pCreds[i]->CredentialBlobSize / sizeof(wchar_t));
                    size_t pipe = blob.find(L'|');
                    if (pipe != std::wstring::npos) {
                        c.password = blob.substr(0, pipe);
                        c.keyPath = blob.substr(pipe + 1);
                    } else {
                        c.password = blob;
                    }
                }
                creds.push_back(c);
            }
        }
        CredFree(pCreds);
    }
    return creds;
}

void CredentialManager::SaveCredential(const Credential& cred) {
    CREDENTIALW c = { 0 };
    c.Type = CRED_TYPE_GENERIC;
    std::wstring target = PREFIX + cred.alias;
    c.TargetName = (LPWSTR)target.c_str();
    c.UserName = (LPWSTR)cred.username.c_str();
    c.Persist = CRED_PERSIST_LOCAL_MACHINE;

    std::wstring blob = cred.password + L"|" + cred.keyPath;
    c.CredentialBlobSize = (DWORD)(blob.length() * sizeof(wchar_t));
    c.CredentialBlob = (LPBYTE)blob.c_str();

    CredWriteW(&c, 0);
}

void CredentialManager::DeleteCredential(const std::wstring& alias) {
    std::wstring target = PREFIX + alias;
    CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0);
}

Credential CredentialManager::GetCredential(const std::wstring& alias) {
    std::wstring target = PREFIX + alias;
    PCREDENTIALW pCred = NULL;
    Credential c;
    
    if (CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &pCred)) {
        c.alias = alias;
        c.username = pCred->UserName ? pCred->UserName : L"";
        if (pCred->CredentialBlobSize > 0) {
            std::wstring blob((wchar_t*)pCred->CredentialBlob, pCred->CredentialBlobSize / sizeof(wchar_t));
            size_t pipe = blob.find(L'|');
            if (pipe != std::wstring::npos) {
                c.password = blob.substr(0, pipe);
                c.keyPath = blob.substr(pipe + 1);
            } else {
                c.password = blob;
            }
        }
        CredFree(pCred);
    }
    return c;
}
