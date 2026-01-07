#include "MainWindow.h"
#include "ProcessUtils.h"
#include <commdlg.h>

void MainWindow::OnNewConnection()
{
    Connection n; n.port = L"22"; n.user = L"root";
    if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONNECTION), m_hwnd, ConnectionDialogProc, (LPARAM)&n) == IDOK) {
        if (n.name.empty()) n.name = L"Unnamed";
        m_allConnections.push_back(n); 
        ConnectionManager::SaveConnections(m_allConnections); 
        ReloadConnections();
    }
}

void MainWindow::OnEditConnection()
{
    TVITEM tvi = { 0 }; tvi.hItem = TreeView_GetSelection(m_hTreeView); tvi.mask = TVIF_PARAM;
    if (TreeView_GetItem(m_hTreeView, &tvi) && tvi.lParam != (LPARAM)-1) {
        size_t idx = (size_t)tvi.lParam;
        if (idx < m_filteredIndices.size()) {
            size_t realIdx = m_filteredIndices[idx];
            if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONNECTION), m_hwnd, ConnectionDialogProc, (LPARAM)&m_allConnections[realIdx]) == IDOK) {
                ConnectionManager::SaveConnections(m_allConnections); 
                ReloadConnections();
            }
        }
    }
}

void MainWindow::OnDeleteConnection()
{
    TVITEM tvi = { 0 }; tvi.hItem = TreeView_GetSelection(m_hTreeView); tvi.mask = TVIF_PARAM;
    if (TreeView_GetItem(m_hTreeView, &tvi) && tvi.lParam != (LPARAM)-1) {
        size_t idx = (size_t)tvi.lParam;
        if (idx < m_filteredIndices.size()) {
            if (MessageBox(m_hwnd, L"Are you sure you want to delete this connection?", L"Confirm Delete", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                m_allConnections.erase(m_allConnections.begin() + m_filteredIndices[idx]); 
                ConnectionManager::SaveConnections(m_allConnections); 
                ReloadConnections();
            }
        }
    }
}

void MainWindow::OnCloneConnection()
{
    TVITEM tvi = { 0 }; tvi.hItem = TreeView_GetSelection(m_hTreeView); tvi.mask = TVIF_PARAM;
    if (TreeView_GetItem(m_hTreeView, &tvi) && tvi.lParam != (LPARAM)-1) {
        size_t idx = (size_t)tvi.lParam;
        if (idx < m_filteredIndices.size()) {
            Connection cl = m_allConnections[m_filteredIndices[idx]]; 
            cl.name += L" (Copy)";
            if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONNECTION), m_hwnd, ConnectionDialogProc, (LPARAM)&cl) == IDOK) {
                m_allConnections.push_back(cl); 
                ConnectionManager::SaveConnections(m_allConnections); 
                ReloadConnections();
            }
        }
    }
}

void MainWindow::OnDuplicateSession()
{
    int sel = TabCtrl_GetCurSel(m_hTabControl);
    if (sel != -1) {
        TCITEM tie; tie.mask = TCIF_PARAM;
        if (TabCtrl_GetItem(m_hTabControl, sel, &tie)) {
            Session* s = (Session*)tie.lParam;
            LaunchSession(s->conn);
        }
    }
}

void MainWindow::OnQuickConnect()
{
    Connection t; t.port = L"22"; t.user = L"root";
    if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_QUICK_CONNECT), m_hwnd, QuickConnectDialogProc, (LPARAM)&t) == IDOK) {
        LaunchSession(t);
    }
}

void MainWindow::OnSettings()
{
    SettingsData d = { m_puttyPath, m_winscpPath };
    if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SETTINGS), m_hwnd, SettingsDialogProc, (LPARAM)&d) == IDOK) {
        m_puttyPath = d.puttyPath; m_winscpPath = d.winscpPath;
        ConnectionManager::SavePuttyPath(m_puttyPath); ConnectionManager::SaveWinSCPPath(m_winscpPath);
    }
}

void MainWindow::ToggleBroadcast()
{
    m_broadcastMode = !m_broadcastMode;
    HMENU hMenu = GetMenu(m_hwnd);
    HMENU hTools = GetSubMenu(hMenu, 2);
    CheckMenuItem(hTools, IDM_TOOLS_MULTI_INPUT, m_broadcastMode ? MF_CHECKED : MF_UNCHECKED);
    
    // External hook defined in InputHook.cpp
    extern HHOOK g_hKeyboardHook;
    extern LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    if (m_broadcastMode) {
        if (!g_hKeyboardHook) {
            g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, GetModuleHandle(NULL), 0);
        }
        SendMessage(m_hStatusBar, SB_SETTEXT, 0, (LPARAM)L"*** BROADCAST MODE ACTIVE ***");
    } else {
        if (g_hKeyboardHook) {
            UnhookWindowsHookEx(g_hKeyboardHook);
            g_hKeyboardHook = NULL;
        }
        SendMessage(m_hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Broadcast Disabled");
    }
}
