#include "ConnectionManager.h"
#include <shlobj.h>
#include <sstream>
#include <fstream>
#include <regex>

#pragma comment(lib, "shell32.lib")

std::wstring ConnectionManager::GetConfigPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        std::wstring wpath(path);
        wpath += L"\\LibreTerm";
        CreateDirectory(wpath.c_str(), NULL);
        return wpath + L"\\connections.ini";
    }
    return L"connections.ini";
}

std::vector<Connection> ConnectionManager::LoadConnections() {
    std::vector<Connection> conns;
    std::wstring path = GetConfigPath();

    int count = GetPrivateProfileInt(L"Sessions", L"Count", 0, path.c_str());

    for (int i = 0; i < count; ++i) {
        std::wstringstream ss;
        ss << L"Session_" << i;
        std::wstring section = ss.str();

        wchar_t buf[256];
        Connection c;
        
        GetPrivateProfileString(section.c_str(), L"Name", L"", buf, 256, path.c_str());
        c.name = buf;
        if (c.name.empty()) continue; 

        GetPrivateProfileString(section.c_str(), L"Host", L"", buf, 256, path.c_str());
        c.host = buf;

        GetPrivateProfileString(section.c_str(), L"Port", L"22", buf, 256, path.c_str());
        c.port = buf;

        GetPrivateProfileString(section.c_str(), L"User", L"", buf, 256, path.c_str());
        c.user = buf;

        GetPrivateProfileString(section.c_str(), L"Password", L"", buf, 256, path.c_str());
        c.password = buf;

        GetPrivateProfileString(section.c_str(), L"Args", L"", buf, 256, path.c_str());
        c.args = buf;

        GetPrivateProfileString(section.c_str(), L"Group", L"", buf, 256, path.c_str());
        c.group = buf;

        conns.push_back(c);
    }
    return conns;
}

void ConnectionManager::SaveConnections(const std::vector<Connection>& conns) {
    std::wstring path = GetConfigPath();
    
    std::wstring countStr = std::to_wstring(conns.size());
    WritePrivateProfileString(L"Sessions", L"Count", countStr.c_str(), path.c_str());

    for (size_t i = 0; i < conns.size(); ++i) {
        std::wstringstream ss;
        ss << L"Session_" << i;
        std::wstring section = ss.str();

        WritePrivateProfileString(section.c_str(), L"Name", conns[i].name.c_str(), path.c_str());
        WritePrivateProfileString(section.c_str(), L"Host", conns[i].host.c_str(), path.c_str());
        WritePrivateProfileString(section.c_str(), L"Port", conns[i].port.c_str(), path.c_str());
        WritePrivateProfileString(section.c_str(), L"User", conns[i].user.c_str(), path.c_str());
        WritePrivateProfileString(section.c_str(), L"Password", conns[i].password.c_str(), path.c_str());
        WritePrivateProfileString(section.c_str(), L"Args", conns[i].args.c_str(), path.c_str());
        WritePrivateProfileString(section.c_str(), L"Group", conns[i].group.c_str(), path.c_str());
    }
}

bool ConnectionManager::ExportToJson(const std::wstring& filePath, const std::vector<Connection>& conns) {
    std::wofstream file(filePath);
    if (!file.is_open()) return false;
    file << L"[\n";
    for (size_t i = 0; i < conns.size(); ++i) {
        file << L"  {\n";
        file << L"    \"name\": \"" << conns[i].name << L"\",\n";
        file << L"    \"group\": \"" << conns[i].group << L"\",\n";
        file << L"    \"host\": \"" << conns[i].host << L"\",\n";
        file << L"    \"port\": \"" << conns[i].port << L"\",\n";
        file << L"    \"user\": \"" << conns[i].user << L"\",\n";
        file << L"    \"password\": \"" << conns[i].password << L"\",\n";
        file << L"    \"args\": \"" << conns[i].args << L"\"\n";
        file << L"  }" << (i == conns.size() - 1 ? L"" : L",") << L"\n";
    }
    file << L"]\n";
    return true;
}

std::vector<Connection> ConnectionManager::ImportFromJson(const std::wstring& filePath) {
    std::vector<Connection> conns;
    std::wifstream file(filePath);
    if (!file.is_open()) return conns;
    std::wstringstream buffer; buffer << file.rdbuf();
    std::wstring content = buffer.str();
    std::wregex obj_re(LR"(\{([^\}]*)\})");
    std::wregex field_re(LR"(\"([^\"]+)\"\s*:\s*\"([^\"]*)\")");
    auto objs_begin = std::wsregex_iterator(content.begin(), content.end(), obj_re);
    auto objs_end = std::wsregex_iterator();
    for (auto i = objs_begin; i != objs_end; ++i) {
        std::wstring obj_content = (*i)[1].str();
        Connection c;
        auto fields_begin = std::wsregex_iterator(obj_content.begin(), obj_content.end(), field_re);
        auto fields_end = std::wsregex_iterator();
        for (auto j = fields_begin; j != fields_end; ++j) {
            std::wstring key = (*j)[1].str();
            std::wstring val = (*j)[2].str();
            if (key == L"name") c.name = val;
            else if (key == L"group") c.group = val;
            else if (key == L"host") c.host = val;
            else if (key == L"port") c.port = val;
            else if (key == L"user") c.user = val;
            else if (key == L"password") c.password = val;
            else if (key == L"args") c.args = val;
        }
        if (!c.name.empty()) conns.push_back(c);
    }
    return conns;
}

std::wstring ConnectionManager::LoadPuttyPath() {
    wchar_t buf[MAX_PATH];
    std::wstring path = GetConfigPath();
    GetPrivateProfileString(L"Settings", L"PuttyPath", L"putty.exe", buf, MAX_PATH, path.c_str());
    return buf;
}

void ConnectionManager::SavePuttyPath(const std::wstring& puttyPath) {
    std::wstring path = GetConfigPath();
    WritePrivateProfileString(L"Settings", L"PuttyPath", puttyPath.c_str(), path.c_str());
}



std::wstring ConnectionManager::LoadWinSCPPath() {

    wchar_t buf[MAX_PATH];

    std::wstring path = GetConfigPath();

    GetPrivateProfileString(L"Settings", L"WinSCPPath", L"winscp.exe", buf, MAX_PATH, path.c_str());

    return buf;

}



void ConnectionManager::SaveWinSCPPath(const std::wstring& winscpPath) {



    std::wstring path = GetConfigPath();



    WritePrivateProfileString(L"Settings", L"WinSCPPath", winscpPath.c_str(), path.c_str());



}




ConnectionManager::WindowState ConnectionManager::LoadWindowState() {



    WindowState state = { CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, false, 250 };



    std::wstring path = GetConfigPath();



    



    state.x = GetPrivateProfileInt(L"Window", L"X", CW_USEDEFAULT, path.c_str());



    state.y = GetPrivateProfileInt(L"Window", L"Y", CW_USEDEFAULT, path.c_str());



    state.width = GetPrivateProfileInt(L"Window", L"Width", CW_USEDEFAULT, path.c_str());



    state.height = GetPrivateProfileInt(L"Window", L"Height", CW_USEDEFAULT, path.c_str());



    state.maximized = GetPrivateProfileInt(L"Window", L"Maximized", 0, path.c_str()) != 0;



    state.sidebarWidth = GetPrivateProfileInt(L"Window", L"SidebarWidth", 250, path.c_str());



    



    return state;





}


void ConnectionManager::SaveWindowState(const WindowState& state) {



    std::wstring path = GetConfigPath();



    WritePrivateProfileString(L"Window", L"X", std::to_wstring(state.x).c_str(), path.c_str());



    WritePrivateProfileString(L"Window", L"Y", std::to_wstring(state.y).c_str(), path.c_str());



    WritePrivateProfileString(L"Window", L"Width", std::to_wstring(state.width).c_str(), path.c_str());



    WritePrivateProfileString(L"Window", L"Height", std::to_wstring(state.height).c_str(), path.c_str());



    WritePrivateProfileString(L"Window", L"Maximized", state.maximized ? L"1" : L"0", path.c_str());



    WritePrivateProfileString(L"Window", L"SidebarWidth", std::to_wstring(state.sidebarWidth).c_str(), path.c_str());



}