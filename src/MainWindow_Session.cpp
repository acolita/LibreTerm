#include "MainWindow.h"
#include "ProcessUtils.h"
#include "CredentialManager.h"

void MainWindow::LaunchSession(const Connection& conn)
{
    std::wstring cmd = m_puttyPath;
    if (cmd.find(L" ") != std::wstring::npos && cmd.front() != L'"') {
        cmd = L"\"" + cmd + L"\"";
    }
    
    // Resolve Credential
    std::wstring user = conn.user;
    std::wstring password = conn.password;
    if (!conn.credentialAlias.empty()) {
        Credential c = CredentialManager::GetCredential(conn.credentialAlias);
        if (!c.alias.empty()) {
            user = c.username;
            password = c.password;
        }
    }
    
    // Options first
    cmd += L" -ssh"; 
    if (!password.empty()) cmd += L" -pw \"" + password + L"\"";
    if (!conn.port.empty()) cmd += L" -P " + conn.port;
    if (!conn.args.empty()) cmd += L" " + conn.args;
    
    // Host last
    cmd += L" ";
    if (!user.empty()) cmd += user + L"@";
    cmd += conn.host;

    DWORD pid = ProcessUtils::LaunchProcess(cmd);
    if (pid == 0) {
        MessageBox(m_hwnd, (L"Failed to launch: " + cmd).c_str(), L"Error", MB_ICONERROR);
        return;
    }

    HWND hPutty = ProcessUtils::WaitForWindow(pid);
    if (hPutty)
    {
        ProcessUtils::EmbedWindow(hPutty, m_hTabControl);
        
        // Store connection info for duplication
        Session* s = new Session{ hPutty, pid, conn.name, conn };
        m_sessions.push_back(s);
        
        TCITEM tie; tie.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE; 
        tie.pszText = (LPWSTR)s->name.c_str(); 
        tie.lParam = (LPARAM)s; 
        tie.iImage = 0; // Terminal icon
        
        int idx = TabCtrl_InsertItem(m_hTabControl, TabCtrl_GetItemCount(m_hTabControl), &tie);
        TabCtrl_SetCurSel(m_hTabControl, idx);
        OnTabChange();
        
        // Ensure focus
        SetFocus(hPutty);
    }
}

void MainWindow::LaunchWinSCP(const Connection& conn)
{
    std::wstring cmd = m_winscpPath;
    if (cmd.find(L" ") != std::wstring::npos && cmd.front() != L'"') {
        cmd = L"\"" + cmd + L"\"";
    }
    
    // Resolve Credential
    std::wstring user = conn.user;
    std::wstring password = conn.password;
    if (!conn.credentialAlias.empty()) {
        Credential c = CredentialManager::GetCredential(conn.credentialAlias);
        if (!c.alias.empty()) {
            user = c.username;
            password = c.password;
        }
    }

    // Format: sftp://user:pass@host:port/
    cmd += L" sftp://"; 
    if (!user.empty()) cmd += user + L"@";
    cmd += conn.host; 
    if (!conn.port.empty()) cmd += L":" + conn.port;
    cmd += L"/";
    
    if (!password.empty()) {
        cmd += L" /password=\"" + password + L"\"";
    }

    if (ProcessUtils::LaunchProcess(cmd) == 0) {
        MessageBox(m_hwnd, (L"Failed to launch: " + cmd).c_str(), L"Error", MB_ICONERROR);
    }
}
void MainWindow::OnTabChange()
{
    int sel = TabCtrl_GetCurSel(m_hTabControl);
    int count = TabCtrl_GetItemCount(m_hTabControl);
    for (int i = 0; i < count; ++i) {
        TCITEM tie; tie.mask = TCIF_PARAM;
        if (TabCtrl_GetItem(m_hTabControl, i, &tie)) {
            Session* s = (Session*)tie.lParam;
            if (i == sel) {
                ShowWindow(s->hEmbedded, SW_SHOW);
                ResizeSession(s);
                SetFocus(s->hEmbedded);
                UpdateStatusBar();
            } else {
                ShowWindow(s->hEmbedded, SW_HIDE);
            }
        }
    }
}

void MainWindow::CloseTab(int index)
{
    if (index < 0 || index >= TabCtrl_GetItemCount(m_hTabControl)) return;
    TCITEM tie; tie.mask = TCIF_PARAM;
    if (TabCtrl_GetItem(m_hTabControl, index, &tie)) {
        Session* s = (Session*)tie.lParam;
        
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, s->pid);
        if (hProcess) {
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
        }

        for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
            if (*it == s) { delete s; m_sessions.erase(it); break; }
        }
        TabCtrl_DeleteItem(m_hTabControl, index);
        
        // Select new tab logic
        int count = TabCtrl_GetItemCount(m_hTabControl);
        if (count > 0) {
            int newSel = (index >= count) ? count - 1 : index;
            TabCtrl_SetCurSel(m_hTabControl, newSel);
        }
        
        OnTabChange();
    }
}

void MainWindow::ResizeSession(Session* session)
{
    if (IsWindow(session->hEmbedded)) {
        HWND hParent = GetParent(session->hEmbedded);
        RECT rc;
        GetClientRect(hParent, &rc);
        
        if (hParent == m_hTabControl) {
            TabCtrl_AdjustRect(m_hTabControl, FALSE, &rc);
        }
        
        MoveWindow(session->hEmbedded, 
            rc.left, rc.top, 
            rc.right - rc.left, rc.bottom - rc.top, 
            TRUE);
    }
}

bool MainWindow::IsSourceOfFocus() {
    HWND hForeground = GetForegroundWindow();
    if (hForeground != m_hwnd && !IsChild(m_hwnd, hForeground)) {
        if (hForeground != m_hwnd) return false; 
    }

    for (Session* s : m_sessions) {
        DWORD dwThreadId = GetWindowThreadProcessId(s->hEmbedded, NULL);
        GUITHREADINFO gti = { sizeof(GUITHREADINFO) };
        if (GetGUIThreadInfo(dwThreadId, &gti)) {
            if (gti.hwndFocus) return true;
        }
    }
    return false;
}

void MainWindow::BroadcastKey(UINT msg, WPARAM wParam, LPARAM lParam) {
    // 1. Identify Source
    Session* pSource = NULL;
    for (Session* s : m_sessions) {
        DWORD dwThreadId = GetWindowThreadProcessId(s->hEmbedded, NULL);
        GUITHREADINFO gti = { sizeof(GUITHREADINFO) };
        if (GetGUIThreadInfo(dwThreadId, &gti)) {
            if (gti.hwndFocus) {
                pSource = s;
                break;
            }
        }
    }

    if (!pSource) return;

    // 2. Broadcast
    for (Session* s : m_sessions) {
        if (s != pSource) {
            HWND hTarget = GetWindow(s->hEmbedded, GW_CHILD);
            if (!hTarget) hTarget = s->hEmbedded;
            PostMessage(hTarget, msg, wParam, lParam);
        }
    }
}
