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
    
    // Jump Server Logic
    if (!conn.jumpHostConnectionName.empty()) {
        // Find Jump Host Connection
        Connection jumpConn;
        bool found = false;
        // Need access to m_allConnections or load them.
        // Better to reload to be safe or use what we have in memory if available. 
        // MainWindow has m_allConnections.
        for (const auto& c : m_allConnections) {
            if (c.name == conn.jumpHostConnectionName) {
                jumpConn = c;
                found = true;
                break;
            }
        }
        
        if (found) {
            // Resolve Jump Credential
            std::wstring jUser = jumpConn.user;
            std::wstring jPass = jumpConn.password;
            if (!jumpConn.credentialAlias.empty()) {
                Credential c = CredentialManager::GetCredential(jumpConn.credentialAlias);
                if (!c.alias.empty()) {
                    jUser = c.username;
                    jPass = c.password;
                }
            }
            
            // Construct Plink Command
            // plink.exe -ssh -l user -pw pass host -nc target_host:target_port
            std::wstring plinkCmd = m_plinkPath;
            if (plinkCmd.empty()) plinkCmd = L"plink.exe";
            
            if (plinkCmd.find(L" ") != std::wstring::npos && plinkCmd.front() != L'"') {
                plinkCmd = L"\"" + plinkCmd + L"\"";
            }
            
            plinkCmd += L" -ssh";
            if (!jUser.empty()) plinkCmd += L" -l " + jUser;
            if (!jPass.empty()) plinkCmd += L" -pw \"" + jPass + L"\"";
            if (!jumpConn.port.empty()) plinkCmd += L" -P " + jumpConn.port;
            
            plinkCmd += L" " + jumpConn.host;
            
            // Netcat mode to target
            // We need target host and port. 
            // PuTTY's -proxycmd replaces %host and %port but using -nc directly with Plink is cleaner if we know them.
            // Actually, PuTTY docs say -proxycmd command uses %host and %port placeholders.
            // Let's use placeholders so Plink connects to the *target* specified by PuTTY.
            // Wait, plink -nc dest:port creates a tunnel.
            // The command should be: plink [jump_opts] jump_host -nc %host:%port
            
            plinkCmd += L" -nc %host:%port";
            
            // Add -proxycmd to PuTTY command
            cmd += L" -proxycmd \"" + plinkCmd + L"\"";
        }
    }
    
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
        // 1. Check for Security Alert
        // PuTTY might show a "PuTTY Security Alert" dialog before the main window is ready
        // or shortly after. WaitForWindow already found a visible window, but it might be the alert.
        wchar_t title[256];
        GetWindowText(hPutty, title, 256);
        if (wcscmp(title, L"PuTTY Security Alert") == 0) {
            // It's the alert! Send 'y' to accept.
            // Using PostMessage to send 'y' (0x59)
            PostMessage(hPutty, WM_CHAR, 'y', 0);
            
            // Now wait for the ACTUAL terminal window
            hPutty = ProcessUtils::WaitForWindow(pid, 5000); 
        }

        if (hPutty) {
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
    
    // Accept all host keys
    cmd += L" -hostkey=\"*\"";

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
