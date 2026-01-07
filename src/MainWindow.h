#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include "resource.h"
#include "ConnectionManager.h"

#pragma comment(lib, "comctl32.lib")

struct Session {
    HWND hEmbedded;
    DWORD pid;
    std::wstring name;
    Connection conn;
};

class MainWindow
{
public:
    MainWindow();
    ~MainWindow();

    BOOL Create(PCWSTR lpWindowName, DWORD dwStyle, DWORD dwExStyle = 0,
        int x = CW_USEDEFAULT, int y = CW_USEDEFAULT,
        int nWidth = CW_USEDEFAULT, int nHeight = CW_USEDEFAULT,
        HWND hWndParent = 0, HMENU hMenu = 0);

    void Show(int nCmdShow);
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    static INT_PTR CALLBACK ConnectionDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK QuickConnectDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    struct SettingsData {
        std::wstring puttyPath;
        std::wstring winscpPath;
    };

    bool IsBroadcastActive() const { return m_broadcastMode; }
    bool IsSourceOfFocus();
    void BroadcastKey(UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND m_hwnd;
    HWND m_hTreeView;
    HWND m_hSearchEdit;
    HWND m_hTabControl;
    HWND m_hStatusBar;

    HIMAGELIST m_hImageList;
    HIMAGELIST m_hTabImageList;

    // Session management
    std::vector<Session*> m_sessions;
    std::vector<size_t> m_filteredIndices;
    std::vector<Connection> m_allConnections;

    // Layout


    int m_treeWidth;
    bool m_isResizing;
    bool m_broadcastMode;
    HTREEITEM m_hDragItem;
    std::wstring m_puttyPath;
    std::wstring m_winscpPath;

    void OnCreate();
    void OnSize(int width, int height);
    void OnDestroy();
    LRESULT OnNotify(LPARAM lParam);
    void OnTabChange();
    void OnTimer();
    void CloseTab(int index);

    static void RegisterWindowClass();

    void LaunchSession(const Connection& conn);
    void LaunchWinSCP(const Connection& conn);
    void ResizeSession(Session* session);
    
    void ReloadConnections();
    void OnNewConnection();
    void OnEditConnection();
    void OnDeleteConnection();
    void OnCloneConnection();
    void OnDuplicateSession();
    void OnQuickConnect();
    void OnSettings();
    void FilterConnections(const std::wstring& query);
    void ToggleBroadcast();
};

LRESULT CALLBACK EditCtrlSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT CALLBACK TabCtrlSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
void SubclassEdit(HWND hDlg, int nIDDlgItem);