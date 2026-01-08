#include "SnippetManager.h"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>

std::wstring SnippetManager::GetConfigPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        std::wstring p(path);
        p += L"\\LibreTerm";
        CreateDirectory(p.c_str(), NULL);
        return p + L"\\snippets.ini";
    }
    return L"snippets.ini";
}

// Simple INI-like format:
// [Count]
// Num=X
// [SnippetN]
// Name=...
// Content=... (escaped newlines?)
// For simplicity, let's use a custom delimiter format or just one-liners for now.
// Since we want multiline, let's use a simple binary-safe or delimiter based approach or just JSON if we had a lib.
// We'll stick to a simple custom format:
// NAME|CONTENT_ESCAPED

std::wstring Escape(const std::wstring& s) {
    std::wstring r;
    for (wchar_t c : s) {
        if (c == L'\n') r += L"\\n";
        else if (c == L'\r') r += L"\\r";
        else if (c == L'\\') r += L"\\\\";
        else r += c;
    }
    return r;
}

std::wstring Unescape(const std::wstring& s) {
    std::wstring r;
    for (size_t i = 0; i < s.length(); ++i) {
        if (s[i] == L'\\' && i + 1 < s.length()) {
            wchar_t n = s[i + 1];
            if (n == L'n') r += L'\n';
            else if (n == L'r') r += L'\r';
            else if (n == L'\\') r += L'\\';
            else r += n; // Unknown escape
            i++;
        } else {
            r += s[i];
        }
    }
    return r;
}

std::vector<Snippet> SnippetManager::LoadSnippets() {
    std::vector<Snippet> out;
    std::wifstream f(GetConfigPath());
    if (!f.is_open()) return out;

    std::wstring line;
    while (std::getline(f, line)) {
        size_t pipe = line.find(L'|');
        if (pipe != std::wstring::npos) {
            Snippet s;
            s.name = line.substr(0, pipe);
            s.content = Unescape(line.substr(pipe + 1));
            out.push_back(s);
        }
    }
    return out;
}

void SnippetManager::SaveSnippets(const std::vector<Snippet>& snippets) {
    std::wofstream f(GetConfigPath());
    if (!f.is_open()) return;

    for (const auto& s : snippets) {
        f << s.name << L"|" << Escape(s.content) << std::endl;
    }
}
