#pragma once
#include <string>
#include <vector>

struct Snippet {
    std::wstring name;
    std::wstring content;
};

class SnippetManager {
public:
    static std::vector<Snippet> LoadSnippets();
    static void SaveSnippets(const std::vector<Snippet>& snippets);
    static std::wstring GetConfigPath();
};
