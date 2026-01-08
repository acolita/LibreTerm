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
    static INT_PTR CALLBACK CredentialManagerDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK CredentialEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    struct SettingsData {
        std::wstring puttyPath;
        std::wstring winscpPath;
        std::wstring plinkPath;
    };

    bool IsBroadcastActive() const { return m_broadcastMode; }
    bool IsSourceOfFocus();
    void BroadcastKey(UINT msg, WPARAM wParam, LPARAM lParam);

    // v1.1.0 Features
    void ToggleFullscreen();
    void ToggleSidebar();
    void OnRenameTab();
    void CloseOthers(int keepIndex);
    void CloseToRight(int index);
    void UpdateStatusBar();
    static INT_PTR CALLBACK RenameTabDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // v1.2.0 Features
    static INT_PTR CALLBACK SnippetManagerDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK SnippetEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    void SendTextToSession(Session* s, const std::wstring& text);
    void SendSnippet(const std::wstring& text, bool toAll);
    void OnQuickSend();
    void OnSnippetManager();

private:
    HWND m_hwnd;
    HWND m_hTreeView;
    HWND m_hSearchEdit;
    HWND m_hTabControl;
    HWND m_hStatusBar;
    HMENU m_hMenu;

    // Quick Command Bar
    HWND m_hQuickEdit;
    HWND m_hQuickBtn;
    HWND m_hQuickChk;

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
    bool m_isFullscreen;
    bool m_showSidebar;
    HTREEITEM m_hDragItem;
    std::wstring m_puttyPath;
    std::wstring m_winscpPath;
    std::wstring m_plinkPath;

    // Fullscreen state
    DWORD m_oldStyle;
    DWORD m_oldExStyle;
    RECT  m_oldRect;

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
    void OnCredentialManager();
    void FilterConnections(const std::wstring& query);
    void ToggleBroadcast();
};

LRESULT CALLBACK EditCtrlSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT CALLBACK TabCtrlSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
void SubclassEdit(HWND hDlg, int nIDDlgItem);
