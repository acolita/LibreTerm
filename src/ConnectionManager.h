#pragma once
#include <string>
#include <vector>
#include <windows.h>

struct Connection {
    std::wstring name;
    std::wstring host;
    std::wstring port;
    std::wstring user;
    std::wstring password;
    std::wstring args;
    std::wstring group;
};

class ConnectionManager {
public:
    static std::wstring GetConfigPath();
    static std::vector<Connection> LoadConnections();
    static void SaveConnections(const std::vector<Connection>& conns);

    static bool ExportToJson(const std::wstring& filePath, const std::vector<Connection>& conns);
    static std::vector<Connection> ImportFromJson(const std::wstring& filePath);

    static std::wstring LoadPuttyPath();
    static void SavePuttyPath(const std::wstring& path);

    static std::wstring LoadWinSCPPath();
    static void SaveWinSCPPath(const std::wstring& path);

    struct WindowState {
        int x, y, width, height;
        bool maximized;
        int sidebarWidth;
    };
    static WindowState LoadWindowState();
    static void SaveWindowState(const WindowState& state);
};
