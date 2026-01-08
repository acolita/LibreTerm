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

void MainWindow::OnCredentialManager()
{
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CREDENTIAL_MANAGER), m_hwnd, CredentialManagerDialogProc);
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

void MainWindow::ToggleFullscreen()
{
    m_isFullscreen = !m_isFullscreen;
    if (m_isFullscreen) {
        m_oldStyle = GetWindowLong(m_hwnd, GWL_STYLE);
        m_oldExStyle = GetWindowLong(m_hwnd, GWL_EXSTYLE);
        GetWindowRect(m_hwnd, &m_oldRect);

        SetMenu(m_hwnd, NULL);
        SetWindowLong(m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        
        ShowWindow(m_hStatusBar, SW_HIDE);
        ShowWindow(m_hTreeView, SW_HIDE);
        ShowWindow(m_hSearchEdit, SW_HIDE);
        ShowWindow(m_hTabControl, SW_HIDE);

        for (Session* s : m_sessions) {
            SetParent(s->hEmbedded, m_hwnd);
        }

        HMONITOR hMon = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) }; mi.cbSize = sizeof(mi);
        if (GetMonitorInfo(hMon, &mi)) {
            MoveWindow(m_hwnd, mi.rcMonitor.left, mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top, TRUE);
        }
    } else {
        SetWindowLong(m_hwnd, GWL_STYLE, m_oldStyle);
        SetWindowLong(m_hwnd, GWL_EXSTYLE, m_oldExStyle);
        SetMenu(m_hwnd, m_hMenu);

        ShowWindow(m_hStatusBar, SW_SHOW);
        if (m_showSidebar) {
            ShowWindow(m_hTreeView, SW_SHOW);
            ShowWindow(m_hSearchEdit, SW_SHOW);
        }
        ShowWindow(m_hTabControl, SW_SHOW);

        for (Session* s : m_sessions) {
            SetParent(s->hEmbedded, m_hTabControl);
        }

        MoveWindow(m_hwnd, m_oldRect.left, m_oldRect.top,
            m_oldRect.right - m_oldRect.left,
            m_oldRect.bottom - m_oldRect.top, TRUE);
    }
    
    // Resize current session
    int sel = TabCtrl_GetCurSel(m_hTabControl);
    if (sel != -1) {
        TCITEM tie; tie.mask = TCIF_PARAM;
        if (TabCtrl_GetItem(m_hTabControl, sel, &tie)) {
            ResizeSession((Session*)tie.lParam);
        }
    }
}

void MainWindow::ToggleSidebar()
{
    m_showSidebar = !m_showSidebar;
    ShowWindow(m_hTreeView, m_showSidebar ? SW_SHOW : SW_HIDE);
    ShowWindow(m_hSearchEdit, m_showSidebar ? SW_SHOW : SW_HIDE);
    
    RECT rc; GetClientRect(m_hwnd, &rc);
    OnSize(rc.right, rc.bottom);
}

void MainWindow::OnRenameTab()
{
    int sel = TabCtrl_GetCurSel(m_hTabControl);
    if (sel == -1) return;
    
    TCITEM tie; tie.mask = TCIF_PARAM | TCIF_TEXT;
    wchar_t buf[256]; tie.pszText = buf; tie.cchTextMax = 256;
    if (TabCtrl_GetItem(m_hTabControl, sel, &tie)) {
        Session* s = (Session*)tie.lParam;
        if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_RENAME_TAB), m_hwnd, RenameTabDialogProc, (LPARAM)buf) == IDOK) {
            s->name = buf;
            tie.pszText = buf;
            TabCtrl_SetItem(m_hTabControl, sel, &tie);
            UpdateStatusBar();
        }
    }
}

void MainWindow::CloseOthers(int keepIndex)
{
    int count = TabCtrl_GetItemCount(m_hTabControl);
    for (int i = count - 1; i >= 0; --i) {
        if (i != keepIndex) CloseTab(i);
    }
}

void MainWindow::CloseToRight(int index)
{
    int count = TabCtrl_GetItemCount(m_hTabControl);
    for (int i = count - 1; i > index; --i) {
        CloseTab(i);
    }
}

void MainWindow::UpdateStatusBar()
{
    int sel = TabCtrl_GetCurSel(m_hTabControl);
    if (sel != -1) {
        TCITEM tie; tie.mask = TCIF_PARAM;
        if (TabCtrl_GetItem(m_hTabControl, sel, &tie)) {
            Session* s = (Session*)tie.lParam;
            std::wstring txt = L"Connected: " + s->name + L" [" + s->conn.user + L"@" + s->conn.host + L":" + s->conn.port + L"]";
            SendMessage(m_hStatusBar, SB_SETTEXT, 0, (LPARAM)txt.c_str());
        }
    } else {
        SendMessage(m_hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Ready");
    }
}

INT_PTR CALLBACK MainWindow::RenameTabDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static wchar_t* pName = NULL;
    switch (message) {
    case WM_INITDIALOG:
        pName = (wchar_t*)lParam;
        SetDlgItemText(hDlg, IDC_EDIT_NAME, pName);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            GetDlgItemText(hDlg, IDC_EDIT_NAME, pName, 256);
            EndDialog(hDlg, IDOK); return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) { EndDialog(hDlg, IDCANCEL); return (INT_PTR)TRUE; }
        break;
    }
    return (INT_PTR)FALSE;
}

