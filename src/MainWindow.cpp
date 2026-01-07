#include "MainWindow.h"
#include "ProcessUtils.h"
#include <commdlg.h>
#include <shellapi.h>
#include <map>
#include <algorithm>

MainWindow::MainWindow() : m_hwnd(NULL), m_hTreeView(NULL), m_hSearchEdit(NULL), m_hTabControl(NULL), m_hStatusBar(NULL), m_treeWidth(250), m_isResizing(false), m_hImageList(NULL), m_hTabImageList(NULL)
{
}

MainWindow::~MainWindow()
{
    for (Session* s : m_sessions) delete s;
    if (m_hImageList) ImageList_Destroy(m_hImageList);
    if (m_hTabImageList) ImageList_Destroy(m_hTabImageList);
}

void MainWindow::RegisterWindowClass()
{
    static bool registered = false;
    if (registered) return;

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = MainWindow::WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"LIBRETERM_MainWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);
    registered = true;
}

BOOL MainWindow::Create(PCWSTR lpWindowName, DWORD dwStyle, DWORD dwExStyle,
    int x, int y, int nWidth, int nHeight,
    HWND hWndParent, HMENU /*hMenu*/)
{
    RegisterWindowClass();

    HMENU hMenuBar = CreateMenu();
    HMENU hFileMenu = CreateMenu();
    AppendMenu(hFileMenu, MF_STRING, IDM_CTX_NEW, L"&New Connection");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_SETTINGS, L"&Settings");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"E&xit");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");

    HMENU hHelpMenu = CreateMenu();
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hHelpMenu, L"&Help");
    AppendMenu(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, L"&About");

    m_hwnd = CreateWindowEx(dwExStyle, L"LIBRETERM_MainWindow", lpWindowName, dwStyle,
        x, y, nWidth, nHeight, hWndParent, hMenuBar, GetModuleHandle(NULL), this);

    return (m_hwnd ? TRUE : FALSE);
}

void MainWindow::Show(int nCmdShow)
{
    ShowWindow(m_hwnd, nCmdShow);
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MainWindow* pThis = (MainWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (uMsg == WM_NCCREATE)
    {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (MainWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hwnd = hwnd;
    }

    if (pThis)
    {
        switch (uMsg)
        {
        case WM_CREATE:
            pThis->OnCreate();
            return 0;

        case WM_SIZE:
            pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_NOTIFY:
            return pThis->OnNotify(lParam);

        case WM_SETFOCUS:
            {
                int sel = TabCtrl_GetCurSel(pThis->m_hTabControl);
                if (sel != -1) {
                    TCITEM tie; tie.mask = TCIF_PARAM;
                    if (TabCtrl_GetItem(pThis->m_hTabControl, sel, &tie)) {
                        Session* s = (Session*)tie.lParam;
                        if (IsWindow(s->hEmbedded)) {
                            SetFocus(s->hEmbedded);
                        }
                    }
                }
            }
            return 0;

        case WM_COMMAND:
            if (lParam != 0 && (HWND)lParam == pThis->m_hSearchEdit && HIWORD(wParam) == EN_CHANGE) {
                wchar_t buf[256];
                GetWindowText(pThis->m_hSearchEdit, buf, 256);
                pThis->FilterConnections(buf);
                return 0;
            }
            switch (LOWORD(wParam)) {
            case IDM_FILE_EXIT:
                DestroyWindow(hwnd);
                break;
            case IDM_HELP_ABOUT:
                MessageBox(hwnd, L"LibreTerm v0.6.0\n\n- Search Filtering\n- Window State Persistence\n- Plain Text Passwords\n- WinSCP & PuTTY Integration", L"About LibreTerm", MB_OK | MB_ICONINFORMATION);
                break;
            case IDM_CTX_NEW:
                pThis->OnNewConnection();
                break;
            case IDM_FILE_SETTINGS:
                pThis->OnSettings();
                break;
            case IDM_CTX_CONNECT:
                {
                    TVITEM tvi = { 0 };
                    tvi.hItem = TreeView_GetSelection(pThis->m_hTreeView);
                    tvi.mask = TVIF_PARAM;
                    if (TreeView_GetItem(pThis->m_hTreeView, &tvi)) {
                        if (tvi.lParam != (LPARAM)-1) {
                            size_t idx = (size_t)tvi.lParam;
                            if (idx < pThis->m_connections.size()) pThis->LaunchSession(pThis->m_connections[idx]);
                        }
                    }
                }
                break;
            case IDM_CTX_WINSCP:
                {
                    TVITEM tvi = { 0 };
                    tvi.hItem = TreeView_GetSelection(pThis->m_hTreeView);
                    tvi.mask = TVIF_PARAM;
                    if (TreeView_GetItem(pThis->m_hTreeView, &tvi)) {
                        if (tvi.lParam != (LPARAM)-1) {
                            size_t idx = (size_t)tvi.lParam;
                            if (idx < pThis->m_connections.size()) pThis->LaunchWinSCP(pThis->m_connections[idx]);
                        }
                    }
                }
                break;
            case IDM_CTX_EDIT:
                pThis->OnEditConnection();
                break;
            case IDM_CTX_DELETE:
                pThis->OnDeleteConnection();
                break;
            case IDM_CTX_CLONE:
                pThis->OnCloneConnection();
                break;
            case IDM_TAB_CLOSE:
                pThis->CloseTab(TabCtrl_GetCurSel(pThis->m_hTabControl));
                break;
            case IDM_TAB_CLOSE_ALL:
                while (TabCtrl_GetItemCount(pThis->m_hTabControl) > 0) pThis->CloseTab(0);
                break;
            case IDM_TAB_DUPLICATE:
                pThis->OnDuplicateSession();
                break;
            }
            return 0;

        case WM_TIMER:
            pThis->OnTimer();
            return 0;

        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt);
                if (abs(pt.x - pThis->m_treeWidth) < 5) {
                    SetCursor(LoadCursor(NULL, IDC_SIZEWE));
                    return TRUE;
                }
            }
            break;

        case WM_LBUTTONDOWN:
            {
                int x = LOWORD(lParam);
                if (abs(x - pThis->m_treeWidth) < 5) {
                    pThis->m_isResizing = true;
                    SetCapture(hwnd);
                }
            }
            return 0;

        case WM_LBUTTONUP:
            if (pThis->m_isResizing) { pThis->m_isResizing = false; ReleaseCapture(); }
            return 0;

        case WM_MOUSEMOVE:
            if (pThis->m_isResizing) {
                pThis->m_treeWidth = LOWORD(lParam);
                if (pThis->m_treeWidth < 50) pThis->m_treeWidth = 50;
                RECT rc;
                GetClientRect(hwnd, &rc);
                pThis->OnSize(rc.right, rc.bottom);
            }
            return 0;

        case WM_ACTIVATE:
            if (LOWORD(wParam) != WA_INACTIVE) {
                RegisterHotKey(hwnd, 1, MOD_CONTROL, VK_TAB);
                RegisterHotKey(hwnd, 2, MOD_CONTROL | MOD_SHIFT, VK_TAB);
                RegisterHotKey(hwnd, 3, MOD_CONTROL, 'W');
            } else {
                UnregisterHotKey(hwnd, 1);
                UnregisterHotKey(hwnd, 2);
                UnregisterHotKey(hwnd, 3);
            }
            return 0;

        case WM_HOTKEY:
            if (wParam == 1) { // Ctrl+Tab
                int sel = TabCtrl_GetCurSel(pThis->m_hTabControl);
                int count = TabCtrl_GetItemCount(pThis->m_hTabControl);
                if (count > 0) { TabCtrl_SetCurSel(pThis->m_hTabControl, (sel + 1) % count); pThis->OnTabChange(); }
            } else if (wParam == 2) { // Ctrl+Shift+Tab
                int sel = TabCtrl_GetCurSel(pThis->m_hTabControl);
                int count = TabCtrl_GetItemCount(pThis->m_hTabControl);
                if (count > 0) { TabCtrl_SetCurSel(pThis->m_hTabControl, (sel - 1 + count) % count); pThis->OnTabChange(); }
            } else if (wParam == 3) { // Ctrl+W
                pThis->CloseTab(TabCtrl_GetCurSel(pThis->m_hTabControl));
            }
            return 0;

        case WM_MOUSEACTIVATE:
            return MA_ACTIVATE;

        case WM_DESTROY:
            // Save Window State
            WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
            if (GetWindowPlacement(hwnd, &wp)) {
                ConnectionManager::WindowState state;
                state.x = wp.rcNormalPosition.left;
                state.y = wp.rcNormalPosition.top;
                state.width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
                state.height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
                state.maximized = (wp.showCmd == SW_MAXIMIZE);
                state.sidebarWidth = pThis->m_treeWidth;
                ConnectionManager::SaveWindowState(state);
            }

            for (Session* s : pThis->m_sessions) { HANDLE hP = OpenProcess(PROCESS_TERMINATE, FALSE, s->pid); if (hP) { TerminateProcess(hP, 0); CloseHandle(hP); } }
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void MainWindow::OnCreate()
{
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TREEVIEW_CLASSES | ICC_TAB_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    m_hSearchEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        0, 0, 0, 0,
        m_hwnd, (HMENU)IDC_EDIT_SEARCH, GetModuleHandle(NULL), NULL);
    
    // Subclass Search Edit for Ctrl+A
    SetWindowSubclass(m_hSearchEdit, EditCtrlSubclassProc, 0, 0);
    
    // Set a smaller font for search
    SendMessage(m_hSearchEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

    m_hTreeView = CreateWindowEx(0, WC_TREEVIEW, L"Connections",
        WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        0, 0, 0, 0,
        m_hwnd, NULL, GetModuleHandle(NULL), NULL);

    m_hTabControl = CreateWindowEx(0, WC_TABCONTROL, L"",
        WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0, 0, 0, 0,
        m_hwnd, NULL, GetModuleHandle(NULL), NULL);

    m_hStatusBar = CreateWindowEx(0, STATUSCLASSNAME, NULL, 
        WS_VISIBLE | WS_CHILD | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        m_hwnd, NULL, GetModuleHandle(NULL), NULL);

    m_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 1);
    m_hTabImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 1);
    HICON hFolder = NULL; HICON hServer = NULL;
    ExtractIconEx(L"shell32.dll", 3, NULL, &hFolder, 1);
    ExtractIconEx(L"shell32.dll", 15, NULL, &hServer, 1);
    if (hFolder) { ImageList_AddIcon(m_hImageList, hFolder); DestroyIcon(hFolder); }
    if (hServer) { ImageList_AddIcon(m_hImageList, hServer); ImageList_AddIcon(m_hTabImageList, hServer); DestroyIcon(hServer); }
    TreeView_SetImageList(m_hTreeView, m_hImageList, TVSIL_NORMAL);
    TabCtrl_SetImageList(m_hTabControl, m_hTabImageList);

    m_puttyPath = ConnectionManager::LoadPuttyPath();
    m_winscpPath = ConnectionManager::LoadWinSCPPath();
    
    ConnectionManager::WindowState state = ConnectionManager::LoadWindowState();
    if (state.sidebarWidth > 50) m_treeWidth = state.sidebarWidth;
    if (state.x != CW_USEDEFAULT) {
        MoveWindow(m_hwnd, state.x, state.y, state.width, state.height, TRUE);
        if (state.maximized) ShowWindow(m_hwnd, SW_MAXIMIZE);
    }

    ReloadConnections();
    SetTimer(m_hwnd, 1, 1000, NULL);
}

void MainWindow::ReloadConnections()
{
    FilterConnections(L"");
}

void MainWindow::FilterConnections(const std::wstring& query)
{
    TreeView_DeleteAllItems(m_hTreeView);
    m_connections = ConnectionManager::LoadConnections();
    
    if (m_connections.empty() && query.empty()) {
        Connection example; example.name = L"Example Host"; example.host = L"127.0.0.1"; example.port = L"22"; example.user = L"root"; example.group = L"Default";
        m_connections.push_back(example); ConnectionManager::SaveConnections(m_connections);
    }

    std::wstring lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::towlower);

    std::map<std::wstring, HTREEITEM> folders;
    TVINSERTSTRUCT tvis = { 0 }; tvis.hInsertAfter = TVI_LAST; tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    
    for (size_t i = 0; i < m_connections.size(); ++i) {
        std::wstring lowerName = m_connections[i].name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
        std::wstring lowerHost = m_connections[i].host;
        std::transform(lowerHost.begin(), lowerHost.end(), lowerHost.begin(), ::towlower);

        if (!lowerQuery.empty() && lowerName.find(lowerQuery) == std::wstring::npos && lowerHost.find(lowerQuery) == std::wstring::npos) {
            continue;
        }

        HTREEITEM hParent = TVI_ROOT;
        if (!m_connections[i].group.empty()) {
            if (folders.find(m_connections[i].group) == folders.end()) {
                tvis.hParent = TVI_ROOT; tvis.item.pszText = (LPWSTR)m_connections[i].group.c_str(); tvis.item.lParam = (LPARAM)-1; tvis.item.iImage = 0; tvis.item.iSelectedImage = 0;
                folders[m_connections[i].group] = TreeView_InsertItem(m_hTreeView, &tvis); 
            }
            hParent = folders[m_connections[i].group];
        }
        tvis.hParent = hParent; tvis.item.pszText = (LPWSTR)m_connections[i].name.c_str(); tvis.item.lParam = (LPARAM)i; tvis.item.iImage = 1; tvis.item.iSelectedImage = 1;
        TreeView_InsertItem(m_hTreeView, &tvis);

        if (hParent != TVI_ROOT) {
            TreeView_Expand(m_hTreeView, hParent, TVE_EXPAND);
        }
    }
}

INT_PTR CALLBACK MainWindow::ConnectionDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static Connection* pConn = NULL;
    switch (message) {
    case WM_INITDIALOG:
        pConn = (Connection*)lParam;
        SetDlgItemText(hDlg, IDC_EDIT_NAME, pConn->name.c_str());
        SetDlgItemText(hDlg, IDC_EDIT_GROUP, pConn->group.c_str());
        SetDlgItemText(hDlg, IDC_EDIT_HOST, pConn->host.c_str());
        SetDlgItemText(hDlg, IDC_EDIT_PORT, pConn->port.c_str());
        SetDlgItemText(hDlg, IDC_EDIT_USER, pConn->user.c_str());
        SetDlgItemText(hDlg, IDC_EDIT_PASSWORD, pConn->password.c_str());
        SetDlgItemText(hDlg, IDC_EDIT_ARGS, pConn->args.c_str());
        
        SubclassEdit(hDlg, IDC_EDIT_NAME);
        SubclassEdit(hDlg, IDC_EDIT_GROUP);
        SubclassEdit(hDlg, IDC_EDIT_HOST);
        SubclassEdit(hDlg, IDC_EDIT_PORT);
        SubclassEdit(hDlg, IDC_EDIT_USER);
        SubclassEdit(hDlg, IDC_EDIT_PASSWORD);
        SubclassEdit(hDlg, IDC_EDIT_ARGS);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            wchar_t buf[256];
            GetDlgItemText(hDlg, IDC_EDIT_NAME, buf, 256); pConn->name = buf;
            GetDlgItemText(hDlg, IDC_EDIT_GROUP, buf, 256); pConn->group = buf;
            GetDlgItemText(hDlg, IDC_EDIT_HOST, buf, 256); pConn->host = buf;
            GetDlgItemText(hDlg, IDC_EDIT_PORT, buf, 256); pConn->port = buf;
            GetDlgItemText(hDlg, IDC_EDIT_USER, buf, 256); pConn->user = buf;
            GetDlgItemText(hDlg, IDC_EDIT_PASSWORD, buf, 256); pConn->password = buf;
            GetDlgItemText(hDlg, IDC_EDIT_ARGS, buf, 256); pConn->args = buf;
            EndDialog(hDlg, IDOK); return (INT_PTR)TRUE;
        } else if (LOWORD(wParam) == IDCANCEL) { EndDialog(hDlg, IDCANCEL); return (INT_PTR)TRUE; }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK MainWindow::SettingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static SettingsData* pData = NULL;
    switch (message) {
    case WM_INITDIALOG:
        pData = (SettingsData*)lParam;
        SetDlgItemText(hDlg, IDC_EDIT_PUTTY_PATH, pData->puttyPath.c_str());
        SetDlgItemText(hDlg, IDC_EDIT_WINSCP_PATH, pData->winscpPath.c_str());
        
        SubclassEdit(hDlg, IDC_EDIT_PUTTY_PATH);
        SubclassEdit(hDlg, IDC_EDIT_WINSCP_PATH);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            wchar_t buf[MAX_PATH];
            GetDlgItemText(hDlg, IDC_EDIT_PUTTY_PATH, buf, MAX_PATH); pData->puttyPath = buf;
            GetDlgItemText(hDlg, IDC_EDIT_WINSCP_PATH, buf, MAX_PATH); pData->winscpPath = buf;
            EndDialog(hDlg, IDOK); return (INT_PTR)TRUE;
        } else if (LOWORD(wParam) == IDCANCEL) { EndDialog(hDlg, IDCANCEL); return (INT_PTR)TRUE; }
        else if (LOWORD(wParam) == IDC_BUTTON_BROWSE || LOWORD(wParam) == IDC_BUTTON_BROWSE_WINSCP) {
            OPENFILENAME ofn; wchar_t szFile[MAX_PATH] = { 0 }; ZeroMemory(&ofn, sizeof(ofn)); ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hDlg; ofn.lpstrFile = szFile; ofn.nMaxFile = sizeof(szFile); ofn.lpstrFilter = L"Executable Files\0*.exe\0All Files\0*.*\0"; ofn.nFilterIndex = 1; ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            if (GetOpenFileName(&ofn)) SetDlgItemText(hDlg, (LOWORD(wParam) == IDC_BUTTON_BROWSE) ? IDC_EDIT_PUTTY_PATH : IDC_EDIT_WINSCP_PATH, ofn.lpstrFile);
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void MainWindow::OnNewConnection()
{
    Connection newConn; newConn.port = L"22"; newConn.user = L"root";
    if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONNECTION), m_hwnd, ConnectionDialogProc, (LPARAM)&newConn) == IDOK) {
        if (newConn.name.empty()) newConn.name = L"Unnamed";
        m_connections.push_back(newConn); ConnectionManager::SaveConnections(m_connections); ReloadConnections();
    }
}

void MainWindow::OnEditConnection()
{
    TVITEM tvi = { 0 }; tvi.hItem = TreeView_GetSelection(m_hTreeView); tvi.mask = TVIF_PARAM;
    if (TreeView_GetItem(m_hTreeView, &tvi) && tvi.lParam != (LPARAM)-1) {
        size_t idx = (size_t)tvi.lParam;
        if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONNECTION), m_hwnd, ConnectionDialogProc, (LPARAM)&m_connections[idx]) == IDOK) {
            ConnectionManager::SaveConnections(m_connections); ReloadConnections();
        }
    }
}

void MainWindow::OnDeleteConnection()
{
    TVITEM tvi = { 0 }; tvi.hItem = TreeView_GetSelection(m_hTreeView); tvi.mask = TVIF_PARAM;
    if (TreeView_GetItem(m_hTreeView, &tvi) && tvi.lParam != (LPARAM)-1) {
        size_t idx = (size_t)tvi.lParam;
        if (MessageBox(m_hwnd, L"Are you sure you want to delete this connection?", L"Confirm Delete", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            m_connections.erase(m_connections.begin() + idx); ConnectionManager::SaveConnections(m_connections); ReloadConnections();
        }
    }
}

void MainWindow::OnCloneConnection()
{
    TVITEM tvi = { 0 }; tvi.hItem = TreeView_GetSelection(m_hTreeView); tvi.mask = TVIF_PARAM;
    if (TreeView_GetItem(m_hTreeView, &tvi) && tvi.lParam != (LPARAM)-1) {
        size_t idx = (size_t)tvi.lParam;
        Connection clone = m_connections[idx];
        clone.name += L" (Copy)";
        if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONNECTION), m_hwnd, ConnectionDialogProc, (LPARAM)&clone) == IDOK) {
            m_connections.push_back(clone); ConnectionManager::SaveConnections(m_connections); ReloadConnections();
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

void MainWindow::OnSettings()
{
    SettingsData data = { m_puttyPath, m_winscpPath };
    if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SETTINGS), m_hwnd, SettingsDialogProc, (LPARAM)&data) == IDOK) {
        m_puttyPath = data.puttyPath; m_winscpPath = data.winscpPath;
        ConnectionManager::SavePuttyPath(m_puttyPath); ConnectionManager::SaveWinSCPPath(m_winscpPath);
    }
}

LRESULT MainWindow::OnNotify(LPARAM lParam)
{
    LPNMHDR pHdr = (LPNMHDR)lParam;
    if (pHdr->hwndFrom == m_hTreeView) {
        if (pHdr->code == NM_DBLCLK) {
            TVITEM tvi = { 0 }; tvi.hItem = TreeView_GetSelection(m_hTreeView); tvi.mask = TVIF_PARAM;
            if (TreeView_GetItem(m_hTreeView, &tvi) && tvi.lParam != (LPARAM)-1) LaunchSession(m_connections[(size_t)tvi.lParam]);
        } else if (pHdr->code == NM_RCLICK) {
            POINT pt; GetCursorPos(&pt); TVHITTESTINFO ht = { 0 }; ht.pt = pt; ScreenToClient(m_hTreeView, &ht.pt);
            HTREEITEM hItem = TreeView_HitTest(m_hTreeView, &ht);
            if (hItem) {
                TreeView_SelectItem(m_hTreeView, hItem); TVITEM tvi = { 0 }; tvi.hItem = hItem; tvi.mask = TVIF_PARAM; TreeView_GetItem(m_hTreeView, &tvi);
                HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_CONTEXT_MENU)); HMENU hPopup = GetSubMenu(hMenu, 0);
                if (tvi.lParam == (LPARAM)-1) { EnableMenuItem(hPopup, IDM_CTX_CONNECT, MF_BYCOMMAND | MF_GRAYED); EnableMenuItem(hPopup, IDM_CTX_WINSCP, MF_BYCOMMAND | MF_GRAYED); EnableMenuItem(hPopup, IDM_CTX_EDIT, MF_BYCOMMAND | MF_GRAYED); EnableMenuItem(hPopup, IDM_CTX_DELETE, MF_BYCOMMAND | MF_GRAYED); }
                TrackPopupMenu(hPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hwnd, NULL); DestroyMenu(hMenu);
            }
        }
    } else if (pHdr->hwndFrom == m_hTabControl) {
        if (pHdr->code == TCN_SELCHANGE) OnTabChange();
        else if (pHdr->code == NM_RCLICK) {
            POINT pt; GetCursorPos(&pt); TCHITTESTINFO ht = { 0 }; ht.pt = pt; ScreenToClient(m_hTabControl, &ht.pt);
            int idx = TabCtrl_HitTest(m_hTabControl, &ht);
            if (idx != -1) {
                TabCtrl_SetCurSel(m_hTabControl, idx); OnTabChange();
                HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TAB_MENU)); TrackPopupMenu(GetSubMenu(hMenu, 0), TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hwnd, NULL); DestroyMenu(hMenu);
            }
        }
    }
    return 0;
}

void MainWindow::LaunchSession(const Connection& conn)
{
    std::wstring cmd = m_puttyPath; if (cmd.find(L" ") != std::wstring::npos && cmd.front() != L'"') cmd = L"\"" + cmd + L"\"";
    cmd += L" -ssh"; 
    if (!conn.password.empty()) cmd += L" -pw \"" + conn.password + L"\"";
    if (!conn.port.empty()) cmd += L" -P " + conn.port;
    if (!conn.args.empty()) cmd += L" " + conn.args;
    cmd += L" "; if (!conn.user.empty()) cmd += conn.user + L"@";
    cmd += conn.host;

    DWORD pid = ProcessUtils::LaunchProcess(cmd);
    if (pid == 0) { MessageBox(m_hwnd, (L"Failed to launch: " + cmd).c_str(), L"Error", MB_ICONERROR); return; }
    HWND hPutty = ProcessUtils::WaitForWindow(pid);
    if (hPutty) {
        ProcessUtils::EmbedWindow(hPutty, m_hTabControl);
        Session* s = new Session{ hPutty, pid, conn.name, conn }; m_sessions.push_back(s);
        TCITEM tie; tie.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE; tie.pszText = (LPWSTR)s->name.c_str(); tie.lParam = (LPARAM)s; tie.iImage = 0;
        int idx = TabCtrl_InsertItem(m_hTabControl, TabCtrl_GetItemCount(m_hTabControl), &tie);
        TabCtrl_SetCurSel(m_hTabControl, idx); OnTabChange();
    }
}

void MainWindow::LaunchWinSCP(const Connection& conn)
{
    std::wstring cmd = m_winscpPath; if (cmd.find(L" ") != std::wstring::npos && cmd.front() != L'"') cmd = L"\"" + cmd + L"\"";
    cmd += L" sftp://"; if (!conn.user.empty()) cmd += conn.user + L"@";
    cmd += conn.host; if (!conn.port.empty()) cmd += L":" + conn.port;
    cmd += L"/";
    if (!conn.password.empty()) cmd += L" /password=\"" + conn.password + L"\"";
    if (ProcessUtils::LaunchProcess(cmd) == 0) MessageBox(m_hwnd, (L"Failed to launch: " + cmd).c_str(), L"Error", MB_ICONERROR);
}

void MainWindow::OnTabChange()
{
    int sel = TabCtrl_GetCurSel(m_hTabControl); int count = TabCtrl_GetItemCount(m_hTabControl);
    for (int i = 0; i < count; ++i) {
        TCITEM tie; tie.mask = TCIF_PARAM;
        if (TabCtrl_GetItem(m_hTabControl, i, &tie)) {
            Session* s = (Session*)tie.lParam;
            if (i == sel) { ShowWindow(s->hEmbedded, SW_SHOW); ResizeSession(s); SetFocus(s->hEmbedded); SendMessage(m_hStatusBar, SB_SETTEXT, 0, (LPARAM)(L"Connected to " + s->name).c_str()); } 
            else ShowWindow(s->hEmbedded, SW_HIDE);
        }
    }
}

void MainWindow::OnTimer()
{
    std::vector<int> toClose; int count = TabCtrl_GetItemCount(m_hTabControl);
    for (int i = 0; i < count; ++i) {
        TCITEM tie; tie.mask = TCIF_PARAM;
        if (TabCtrl_GetItem(m_hTabControl, i, &tie)) {
            Session* s = (Session*)tie.lParam;
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, s->pid);
            if (hProcess) { DWORD exitCode = 0; if (GetExitCodeProcess(hProcess, &exitCode) && exitCode != STILL_ACTIVE) toClose.push_back(i); CloseHandle(hProcess); }
            else toClose.push_back(i);
        }
    }
    for (int i = (int)toClose.size() - 1; i >= 0; --i) CloseTab(toClose[i]);
}

void MainWindow::CloseTab(int index)
{
    if (index < 0 || index >= TabCtrl_GetItemCount(m_hTabControl)) return;
    TCITEM tie; tie.mask = TCIF_PARAM;
    if (TabCtrl_GetItem(m_hTabControl, index, &tie)) {
        Session* s = (Session*)tie.lParam;
        HANDLE hP = OpenProcess(PROCESS_TERMINATE, FALSE, s->pid); if (hP) { TerminateProcess(hP, 0); CloseHandle(hP); }
        for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) { if (*it == s) { delete s; m_sessions.erase(it); break; } }
        TabCtrl_DeleteItem(m_hTabControl, index); OnTabChange();
    }
}

void MainWindow::ResizeSession(Session* session)
{
    if (IsWindow(session->hEmbedded)) {
        RECT rcTab; GetClientRect(m_hTabControl, &rcTab); TabCtrl_AdjustRect(m_hTabControl, FALSE, &rcTab);
        MoveWindow(session->hEmbedded, rcTab.left, rcTab.top, rcTab.right - rcTab.left, rcTab.bottom - rcTab.top, TRUE);
    }
}

void MainWindow::OnSize(int width, int height)
{
    if (m_hTreeView && m_hTabControl && m_hStatusBar && m_hSearchEdit) {
        SendMessage(m_hStatusBar, WM_SIZE, 0, 0);
        RECT rcStatus; GetWindowRect(m_hStatusBar, &rcStatus);
        int h = height - (rcStatus.bottom - rcStatus.top);
        
        int searchHeight = 22;
        MoveWindow(m_hSearchEdit, 0, 0, m_treeWidth, searchHeight, TRUE);
        MoveWindow(m_hTreeView, 0, searchHeight, m_treeWidth, h - searchHeight, TRUE);
        MoveWindow(m_hTabControl, m_treeWidth + 2, 0, width - m_treeWidth - 2, h, TRUE);
        OnTabChange();
    }
}