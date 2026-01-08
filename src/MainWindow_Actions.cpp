#include "MainWindow.h"
#include "ProcessUtils.h"
#include <commdlg.h>

extern MainWindow* g_pMainWindow;

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

void MainWindow::SendTextToSession(Session* s, const std::wstring& text) {
    if (!s || !IsWindow(s->hEmbedded)) return;
    HWND hTarget = GetWindow(s->hEmbedded, GW_CHILD); // Usually PuTTY's inner window
    if (!hTarget) hTarget = s->hEmbedded;

    for (wchar_t c : text) {
        PostMessage(hTarget, WM_CHAR, (WPARAM)c, 0);
    }
}

void MainWindow::OnQuickSend() {
    wchar_t buf[1024];
    GetWindowText(m_hQuickEdit, buf, 1024);
    std::wstring cmd = buf;
    if (cmd.empty()) return;

    if (SendMessage(m_hQuickChk, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        cmd += L"\r";
    }

    if (m_broadcastMode) {
        for (Session* s : m_sessions) {
            SendTextToSession(s, cmd);
        }
    } else {
        int sel = TabCtrl_GetCurSel(m_hTabControl);
        if (sel != -1) {
            TCITEM tie; tie.mask = TCIF_PARAM;
            if (TabCtrl_GetItem(m_hTabControl, sel, &tie)) {
                Session* s = (Session*)tie.lParam;
                SendTextToSession(s, cmd);
            }
        }
    }
    SetWindowText(m_hQuickEdit, L"");
    SetFocus(m_hQuickEdit);
}

void MainWindow::OnSnippetManager() {
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SNIPPET_MANAGER), m_hwnd, SnippetManagerDialogProc);
}

// Snippet Manager Logic
#include "SnippetManager.h"

INT_PTR CALLBACK MainWindow::SnippetManagerDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static std::vector<Snippet> snippets;
    static HWND hList;

    switch (message) {
    case WM_INITDIALOG:
        hList = GetDlgItem(hDlg, IDC_LIST_SNIPPETS);
        snippets = SnippetManager::LoadSnippets();
        for (const auto& s : snippets) {
            SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)s.name.c_str());
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_SNIP_ADD: {
            Snippet s;
            if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SNIPPET_EDIT), hDlg, SnippetEditDialogProc, (LPARAM)&s) == IDOK) {
                snippets.push_back(s);
                SnippetManager::SaveSnippets(snippets);
                SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)s.name.c_str());
            }
        } break;
        
        case IDC_BTN_SNIP_EDIT: {
            int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SNIPPET_EDIT), hDlg, SnippetEditDialogProc, (LPARAM)&snippets[sel]) == IDOK) {
                    SnippetManager::SaveSnippets(snippets);
                    SendMessage(hList, LB_DELETESTRING, sel, 0);
                    SendMessage(hList, LB_INSERTSTRING, sel, (LPARAM)snippets[sel].name.c_str());
                }
            }
        } break;

        case IDC_BTN_SNIP_DELETE: {
            int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                if (MessageBox(hDlg, L"Delete this snippet?", L"Confirm", MB_YESNO) == IDYES) {
                    snippets.erase(snippets.begin() + sel);
                    SnippetManager::SaveSnippets(snippets);
                    SendMessage(hList, LB_DELETESTRING, sel, 0);
                }
            }
        } break;

        case IDC_BTN_SNIP_SEND:
        case IDC_BTN_SNIP_SEND_ALL: {
            int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR && g_pMainWindow) {
                std::wstring text = snippets[sel].content;
                std::wstring sent = L"";
                for (wchar_t c : text) {
                    if (c == L'\n') sent += L'\r';
                    else sent += c;
                }
                g_pMainWindow->SendSnippet(sent, (LOWORD(wParam) == IDC_BTN_SNIP_SEND_ALL));
            }
        } break;

        case IDCANCEL: EndDialog(hDlg, IDCANCEL); return (INT_PTR)TRUE;
        }
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK MainWindow::SnippetEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static Snippet* pSnip = NULL;
    switch (message) {
    case WM_INITDIALOG:
        pSnip = (Snippet*)lParam;
        SetDlgItemText(hDlg, IDC_EDIT_SNIP_NAME, pSnip->name.c_str());
        SetDlgItemText(hDlg, IDC_EDIT_SNIP_CONTENT, pSnip->content.c_str());
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            wchar_t buf[4096];
            GetDlgItemText(hDlg, IDC_EDIT_SNIP_NAME, buf, 4096); pSnip->name = buf;
            GetDlgItemText(hDlg, IDC_EDIT_SNIP_CONTENT, buf, 4096); pSnip->content = buf;
            EndDialog(hDlg, IDOK); return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) { EndDialog(hDlg, IDCANCEL); return (INT_PTR)TRUE; }
        break;
    }
    return (INT_PTR)FALSE;
}

void MainWindow::SendSnippet(const std::wstring& text, bool toAll) {
    if (toAll) {
        for (Session* s : m_sessions) SendTextToSession(s, text);
    } else {
        int sel = TabCtrl_GetCurSel(m_hTabControl);
        if (sel != -1) {
            TCITEM tie; tie.mask = TCIF_PARAM;
            if (TabCtrl_GetItem(m_hTabControl, sel, &tie)) {
                SendTextToSession((Session*)tie.lParam, text);
            }
        }
    }
}


