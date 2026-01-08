#include "MainWindow.h"
#include "ProcessUtils.h"
#include <commdlg.h>
#include <shellapi.h>
#include <map>
#include <algorithm>

// Global pointer for hook (used in InputHook.cpp)
MainWindow* g_pMainWindow = NULL;

// Helpers in Subclass.cpp
LRESULT CALLBACK EditCtrlSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT CALLBACK TabCtrlSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
void SubclassEdit(HWND hDlg, int nIDDlgItem);

MainWindow::MainWindow() : m_hwnd(NULL), m_hTreeView(NULL), m_hSearchEdit(NULL), m_hTabControl(NULL), m_hStatusBar(NULL), m_treeWidth(250), m_isResizing(false), m_hImageList(NULL), m_hTabImageList(NULL), m_broadcastMode(false), m_hDragItem(NULL), m_isFullscreen(false), m_showSidebar(true), m_oldStyle(0), m_oldExStyle(0), m_hQuickEdit(NULL), m_hQuickBtn(NULL), m_hQuickChk(NULL)
{
    m_oldRect = { 0 };
    g_pMainWindow = this;
}

MainWindow::~MainWindow()
{
    // Unhook logic handled in InputHook but we should ensure it's clean
    // For now, assuming InputHook cleans up or we call ToggleBroadcast(false)
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
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_NEW, L"&New Connection");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_QUICK_CONNECT, L"&Quick Connect\tCtrl+Q");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_IMPORT, L"&Import...");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXPORT, L"&Export...");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_SETTINGS, L"&Settings");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"E&xit");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");

    HMENU hViewMenu = CreateMenu();
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_SIDEBAR, L"Show &Sidebar\tCtrl+B");
    AppendMenu(hViewMenu, MF_STRING, IDM_VIEW_FULLSCREEN, L"&Fullscreen\tF11");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hViewMenu, L"&View");

    HMENU hToolsMenu = CreateMenu();
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hToolsMenu, L"&Tools");
    AppendMenu(hToolsMenu, MF_STRING, IDM_TOOLS_MULTI_INPUT, L"&Multi-Input (Broadcast Mode)");
    AppendMenu(hToolsMenu, MF_STRING, IDM_TOOLS_CREDENTIALS, L"&Credential Manager");
    AppendMenu(hToolsMenu, MF_STRING, IDM_TOOLS_SNIPPETS, L"&Snippet Manager");

    HMENU hHelpMenu = CreateMenu();
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hHelpMenu, L"&Help");
    AppendMenu(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, L"&About");

    m_hMenu = hMenuBar;

    m_hwnd = CreateWindowEx(dwExStyle, L"LIBRETERM_MainWindow", lpWindowName, dwStyle,
        x, y, nWidth, nHeight, hWndParent, m_hMenu, GetModuleHandle(NULL), this);

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
        case WM_CREATE: pThis->OnCreate(); return 0;
        case WM_SIZE: pThis->OnSize(LOWORD(lParam), HIWORD(lParam)); return 0;
        case WM_NOTIFY: return pThis->OnNotify(lParam);
        
        case WM_SETFOCUS:
            {
                int sel = TabCtrl_GetCurSel(pThis->m_hTabControl);
                if (sel != -1) {
                    TCITEM tie; tie.mask = TCIF_PARAM;
                    if (TabCtrl_GetItem(pThis->m_hTabControl, sel, &tie)) {
                        Session* s = (Session*)tie.lParam;
                        if (IsWindow(s->hEmbedded)) SetFocus(s->hEmbedded);
                    }
                }
            }
            return 0;

        case WM_COMMAND:
            if (HIWORD(wParam) == 999 && (HWND)lParam == pThis->m_hTabControl) {
                pThis->CloseTab(LOWORD(wParam));
                return 0;
            }
            if (lParam != 0 && (HWND)lParam == pThis->m_hSearchEdit && HIWORD(wParam) == EN_CHANGE) {
                wchar_t buf[256]; GetWindowText(pThis->m_hSearchEdit, buf, 256);
                pThis->FilterConnections(buf);
                return 0;
            }
            switch (LOWORD(wParam)) {
            case IDM_FILE_EXIT: DestroyWindow(hwnd); break;
            case IDM_HELP_ABOUT: MessageBox(hwnd, L"LibreTerm v0.9.0", L"About", MB_OK); break;
            
            // Handled in MainWindow_Actions.cpp
            case IDM_FILE_NEW: pThis->OnNewConnection(); break;
            case IDM_FILE_SETTINGS: pThis->OnSettings(); break;
            case IDM_FILE_QUICK_CONNECT: pThis->OnQuickConnect(); break;
            
            case IDM_FILE_IMPORT:
                {
                    OPENFILENAME ofn; wchar_t szFile[MAX_PATH] = { 0 }; ZeroMemory(&ofn, sizeof(ofn)); ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd; ofn.lpstrFile = szFile; ofn.nMaxFile = sizeof(szFile); ofn.lpstrFilter = L"JSON Files\0*.json\0All Files\0*.*\0"; ofn.nFilterIndex = 1; ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                    if (GetOpenFileName(&ofn)) {
                        auto imported = ConnectionManager::ImportFromJson(ofn.lpstrFile);
                        if (!imported.empty()) {
                            pThis->m_allConnections.insert(pThis->m_allConnections.end(), imported.begin(), imported.end());
                            ConnectionManager::SaveConnections(pThis->m_allConnections);
                            pThis->ReloadConnections();
                            MessageBox(hwnd, L"Import successful!", L"Success", MB_OK);
                        }
                    }
                }
                break;
            case IDM_FILE_EXPORT:
                {
                    OPENFILENAME ofn; wchar_t szFile[MAX_PATH] = L"connections.json"; ZeroMemory(&ofn, sizeof(ofn)); ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd; ofn.lpstrFile = szFile; ofn.nMaxFile = sizeof(szFile); ofn.lpstrFilter = L"JSON Files\0*.json\0All Files\0*.*\0"; ofn.nFilterIndex = 1; ofn.Flags = OFN_OVERWRITEPROMPT;
                    if (GetSaveFileName(&ofn)) {
                        if (ConnectionManager::ExportToJson(ofn.lpstrFile, pThis->m_allConnections))
                            MessageBox(hwnd, L"Export successful!", L"Success", MB_OK);
                    }
                }
                break;

            case IDM_CTX_CONNECT:
                {
                    TVITEM tvi = { 0 }; tvi.hItem = TreeView_GetSelection(pThis->m_hTreeView); tvi.mask = TVIF_PARAM;
                    if (TreeView_GetItem(pThis->m_hTreeView, &tvi) && tvi.lParam != (LPARAM)-1) {
                        size_t idx = (size_t)tvi.lParam;
                        if (idx < pThis->m_filteredIndices.size()) pThis->LaunchSession(pThis->m_allConnections[pThis->m_filteredIndices[idx]]);
                    }
                }
                break;
            case IDM_CTX_WINSCP:
                {
                    TVITEM tvi = { 0 }; tvi.hItem = TreeView_GetSelection(pThis->m_hTreeView); tvi.mask = TVIF_PARAM;
                    if (TreeView_GetItem(pThis->m_hTreeView, &tvi) && tvi.lParam != (LPARAM)-1) {
                        size_t idx = (size_t)tvi.lParam;
                        if (idx < pThis->m_filteredIndices.size()) pThis->LaunchWinSCP(pThis->m_allConnections[pThis->m_filteredIndices[idx]]);
                    }
                }
                break;

            case IDM_CTX_EDIT: pThis->OnEditConnection(); break;
            case IDM_CTX_DELETE: pThis->OnDeleteConnection(); break;
            case IDM_CTX_CLONE: pThis->OnCloneConnection(); break;
            case IDM_TAB_CLOSE: pThis->CloseTab(TabCtrl_GetCurSel(pThis->m_hTabControl)); break;
            case IDM_TAB_CLOSE_ALL: while (TabCtrl_GetItemCount(pThis->m_hTabControl) > 0) pThis->CloseTab(0); break;
            case IDM_TAB_DUPLICATE: pThis->OnDuplicateSession(); break;
            case IDM_TAB_RENAME: pThis->OnRenameTab(); break;
            case IDM_TAB_CLOSE_OTHERS: pThis->CloseOthers(TabCtrl_GetCurSel(pThis->m_hTabControl)); break;
            case IDM_TAB_CLOSE_RIGHT: pThis->CloseToRight(TabCtrl_GetCurSel(pThis->m_hTabControl)); break;
            case IDM_VIEW_SIDEBAR: pThis->ToggleSidebar(); break;
            case IDM_VIEW_FULLSCREEN: pThis->ToggleFullscreen(); break;
            case IDM_TOOLS_MULTI_INPUT: pThis->ToggleBroadcast(); break;
            case IDM_TOOLS_CREDENTIALS: pThis->OnCredentialManager(); break;
            case IDM_TOOLS_SNIPPETS: pThis->OnSnippetManager(); break;
            case IDC_QUICK_CMD_BTN: pThis->OnQuickSend(); break;
            }
            return 0;

        case WM_TIMER: pThis->OnTimer(); return 0;

        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt);
                if (abs(pt.x - pThis->m_treeWidth) < 5) { SetCursor(LoadCursor(NULL, IDC_SIZEWE)); return TRUE; }
            }
            break;

        case WM_LBUTTONDOWN:
            {
                int x = LOWORD(lParam);
                if (abs(x - pThis->m_treeWidth) < 5) { pThis->m_isResizing = true; SetCapture(hwnd); }
            }
            return 0;

        case WM_LBUTTONUP:
            if (pThis->m_isResizing) { pThis->m_isResizing = false; ReleaseCapture(); }
            if (pThis->m_hDragItem) {
                ReleaseCapture();
                POINT pt; GetCursorPos(&pt); ScreenToClient(pThis->m_hTreeView, &pt);
                TVHITTESTINFO ht = { 0 }; ht.pt = pt;
                HTREEITEM hTarget = TreeView_HitTest(pThis->m_hTreeView, &ht);
                if (hTarget && hTarget != pThis->m_hDragItem) {
                    TVITEM tviTarget = { 0 }; tviTarget.hItem = hTarget; tviTarget.mask = TVIF_PARAM | TVIF_TEXT;
                    wchar_t buf[256]; tviTarget.pszText = buf; tviTarget.cchTextMax = 256;
                    TreeView_GetItem(pThis->m_hTreeView, &tviTarget);
                    
                    TVITEM tviSource = { 0 }; tviSource.hItem = pThis->m_hDragItem; tviSource.mask = TVIF_PARAM;
                    TreeView_GetItem(pThis->m_hTreeView, &tviSource);
                    
                    if (tviSource.lParam != (LPARAM)-1) {
                        std::wstring newGroup = L"";
                        if (tviTarget.lParam == (LPARAM)-1) newGroup = buf;
                        else {
                            HTREEITEM hParent = TreeView_GetParent(pThis->m_hTreeView, hTarget);
                            if (hParent) {
                                TVITEM tviParent = { 0 }; tviParent.hItem = hParent; tviParent.mask = TVIF_TEXT;
                                tviParent.pszText = buf; tviParent.cchTextMax = 256;
                                TreeView_GetItem(pThis->m_hTreeView, &tviParent);
                                newGroup = buf;
                            }
                        }
                        size_t idx = (size_t)tviSource.lParam;
                        if (idx < pThis->m_filteredIndices.size()) {
                            // Find matching connection in m_allConnections via m_filteredIndices
                            // Actually, m_filteredIndices[idx] is the index in m_allConnections
                            size_t realIdx = pThis->m_filteredIndices[idx];
                            pThis->m_allConnections[realIdx].group = newGroup;
                            ConnectionManager::SaveConnections(pThis->m_allConnections);
                            pThis->ReloadConnections();
                        }
                    }
                }
                pThis->m_hDragItem = NULL; TreeView_SelectDropTarget(pThis->m_hTreeView, NULL);
            }
            return 0;

        case WM_MOUSEMOVE:
            if (pThis->m_isResizing) {
                pThis->m_treeWidth = LOWORD(lParam); if (pThis->m_treeWidth < 50) pThis->m_treeWidth = 50;
                RECT rc; GetClientRect(hwnd, &rc); pThis->OnSize(rc.right, rc.bottom);
            }
            if (pThis->m_hDragItem) {
                POINT pt; GetCursorPos(&pt); ScreenToClient(pThis->m_hTreeView, &pt);
                TVHITTESTINFO ht = { 0 }; ht.pt = pt;
                HTREEITEM hTarget = TreeView_HitTest(pThis->m_hTreeView, &ht);
                TreeView_SelectDropTarget(pThis->m_hTreeView, hTarget);
            }
            return 0;

        case WM_ACTIVATE:
            if (LOWORD(wParam) != WA_INACTIVE) {
                RegisterHotKey(hwnd, 1, MOD_CONTROL, VK_TAB);
                RegisterHotKey(hwnd, 2, MOD_CONTROL | MOD_SHIFT, VK_TAB);
                RegisterHotKey(hwnd, 3, MOD_CONTROL, 'W');
                RegisterHotKey(hwnd, 4, MOD_CONTROL, 'Q');
                RegisterHotKey(hwnd, 5, 0, VK_F11);
                RegisterHotKey(hwnd, 6, MOD_CONTROL, 'B');
                RegisterHotKey(hwnd, 7, 0, VK_F2);
            } else {
                UnregisterHotKey(hwnd, 1); UnregisterHotKey(hwnd, 2); UnregisterHotKey(hwnd, 3); UnregisterHotKey(hwnd, 4);
                UnregisterHotKey(hwnd, 5); UnregisterHotKey(hwnd, 6); UnregisterHotKey(hwnd, 7);
            }
            return 0;

        case WM_HOTKEY:
            if (wParam == 1) { 
                int sel = TabCtrl_GetCurSel(pThis->m_hTabControl); int count = TabCtrl_GetItemCount(pThis->m_hTabControl);
                if (count > 0) { TabCtrl_SetCurSel(pThis->m_hTabControl, (sel + 1) % count); pThis->OnTabChange(); }
            } else if (wParam == 2) {
                int sel = TabCtrl_GetCurSel(pThis->m_hTabControl); int count = TabCtrl_GetItemCount(pThis->m_hTabControl);
                if (count > 0) { TabCtrl_SetCurSel(pThis->m_hTabControl, (sel - 1 + count) % count); pThis->OnTabChange(); }
            } else if (wParam == 3) { pThis->CloseTab(TabCtrl_GetCurSel(pThis->m_hTabControl));
            } else if (wParam == 4) { pThis->OnQuickConnect(); 
            } else if (wParam == 5) { pThis->ToggleFullscreen(); 
            } else if (wParam == 6) { pThis->ToggleSidebar(); 
            } else if (wParam == 7) { pThis->OnRenameTab(); }
            return 0;

        case WM_MOUSEACTIVATE: return MA_ACTIVATE;

        case WM_DESTROY: pThis->OnDestroy(); return 0;
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
        m_hwnd, (HMENU)IDC_SEARCH_EDIT, GetModuleHandle(NULL), NULL);
    SetWindowSubclass(m_hSearchEdit, EditCtrlSubclassProc, 0, 0);
    SendMessage(m_hSearchEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

    m_hTreeView = CreateWindowEx(0, WC_TREEVIEW, L"Connections",
        WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        0, 0, 0, 0,
        m_hwnd, (HMENU)IDC_TREEVIEW, GetModuleHandle(NULL), NULL);

    m_hTabControl = CreateWindowEx(0, WC_TABCONTROL, L"",
        WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0, 0, 0, 0,
        m_hwnd, (HMENU)IDC_TABCONTROL, GetModuleHandle(NULL), NULL);
    SetWindowSubclass(m_hTabControl, TabCtrlSubclassProc, 0, 0);

    m_hStatusBar = CreateWindowEx(0, STATUSCLASSNAME, NULL,
        WS_VISIBLE | WS_CHILD | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        m_hwnd, (HMENU)IDC_STATUSBAR, GetModuleHandle(NULL), NULL);

    // Quick Command Bar
    m_hQuickEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
        0, 0, 0, 0,
        m_hwnd, (HMENU)IDC_QUICK_CMD_EDIT, GetModuleHandle(NULL), NULL);
    SendMessage(m_hQuickEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

    m_hQuickBtn = CreateWindowEx(0, L"BUTTON", L"Send",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        0, 0, 0, 0,
        m_hwnd, (HMENU)IDC_QUICK_CMD_BTN, GetModuleHandle(NULL), NULL);
    SendMessage(m_hQuickBtn, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

    m_hQuickChk = CreateWindowEx(0, L"BUTTON", L"Enter",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        0, 0, 0, 0,
        m_hwnd, (HMENU)IDC_QUICK_CMD_CHK, GetModuleHandle(NULL), NULL);
    SendMessage(m_hQuickChk, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SendMessage(m_hQuickChk, BM_SETCHECK, BST_CHECKED, 0);

    m_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 1);
    m_hTabImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 1);
    HICON hF = NULL; HICON hS = NULL;
    ExtractIconEx(L"shell32.dll", 3, NULL, &hF, 1); ExtractIconEx(L"shell32.dll", 15, NULL, &hS, 1);
    if (hF) { ImageList_AddIcon(m_hImageList, hF); DestroyIcon(hF); }
    if (hS) { ImageList_AddIcon(m_hImageList, hS); ImageList_AddIcon(m_hTabImageList, hS); DestroyIcon(hS); }
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
    m_allConnections = ConnectionManager::LoadConnections();
    if (m_allConnections.empty()) {
        Connection ex; ex.name = L"Example Host"; ex.host = L"127.0.0.1"; ex.port = L"22"; ex.user = L"root"; ex.group = L"Default";
        m_allConnections.push_back(ex); ConnectionManager::SaveConnections(m_allConnections);
    }
    // Sort logic
    std::sort(m_allConnections.begin(), m_allConnections.end(), [](const Connection& a, const Connection& b) {
        if (a.group != b.group) return a.group < b.group;
        return a.name < b.name;
    });
    
    // Apply filter
    wchar_t buf[256] = {0};
    if (m_hSearchEdit) GetWindowText(m_hSearchEdit, buf, 256);
    FilterConnections(buf);
}

void MainWindow::FilterConnections(const std::wstring& query)
{
    TreeView_DeleteAllItems(m_hTreeView);
    m_filteredIndices.clear();

    std::wstring lq = query; std::transform(lq.begin(), lq.end(), lq.begin(), ::towlower);
    std::map<std::wstring, HTREEITEM> folders;
    TVINSERTSTRUCT tvis = { 0 }; tvis.hInsertAfter = TVI_LAST; tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    
    for (size_t i = 0; i < m_allConnections.size(); ++i) {
        std::wstring ln = m_allConnections[i].name; std::transform(ln.begin(), ln.end(), ln.begin(), ::towlower);
        std::wstring lh = m_allConnections[i].host; std::transform(lh.begin(), lh.end(), lh.begin(), ::towlower);
        
        if (!lq.empty() && ln.find(lq) == std::wstring::npos && lh.find(lq) == std::wstring::npos) continue;
        
        m_filteredIndices.push_back(i);
        size_t listIdx = m_filteredIndices.size() - 1;

        HTREEITEM hP = TVI_ROOT;
        if (!m_allConnections[i].group.empty()) {
            if (folders.find(m_allConnections[i].group) == folders.end()) {
                tvis.hParent = TVI_ROOT; tvis.item.pszText = (LPWSTR)m_allConnections[i].group.c_str(); tvis.item.lParam = (LPARAM)-1; tvis.item.iImage = 0; tvis.item.iSelectedImage = 0;
                folders[m_allConnections[i].group] = TreeView_InsertItem(m_hTreeView, &tvis);
            }
            hP = folders[m_allConnections[i].group];
        }
        tvis.hParent = hP; tvis.item.pszText = (LPWSTR)m_allConnections[i].name.c_str(); tvis.item.lParam = (LPARAM)listIdx; tvis.item.iImage = 1; tvis.item.iSelectedImage = 1;
        TreeView_InsertItem(m_hTreeView, &tvis);
        
        if (hP != TVI_ROOT) TreeView_Expand(m_hTreeView, hP, TVE_EXPAND);
    }
}

LRESULT MainWindow::OnNotify(LPARAM lParam)
{
    LPNMHDR pH = (LPNMHDR)lParam;
    if (pH->hwndFrom == m_hTreeView) {
        if (pH->code == NM_DBLCLK) {
            TVITEM tvi = { 0 }; tvi.hItem = TreeView_GetSelection(m_hTreeView); tvi.mask = TVIF_PARAM;
            if (TreeView_GetItem(m_hTreeView, &tvi) && tvi.lParam != (LPARAM)-1) {
                size_t idx = (size_t)tvi.lParam;
                if (idx < m_filteredIndices.size()) LaunchSession(m_allConnections[m_filteredIndices[idx]]);
            }
        } else if (pH->code == NM_RCLICK) {
            POINT pt; GetCursorPos(&pt); TVHITTESTINFO ht = { 0 }; ht.pt = pt; ScreenToClient(m_hTreeView, &ht.pt);
            HTREEITEM hI = TreeView_HitTest(m_hTreeView, &ht);
            if (hI) {
                TreeView_SelectItem(m_hTreeView, hI); TVITEM tvi = { 0 }; tvi.hItem = hI; tvi.mask = TVIF_PARAM; TreeView_GetItem(m_hTreeView, &tvi);
                HMENU hM = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_CONTEXT_MENU)); HMENU hP = GetSubMenu(hM, 0);
                if (tvi.lParam == (LPARAM)-1) { EnableMenuItem(hP, IDM_CTX_CONNECT, MF_BYCOMMAND | MF_GRAYED); EnableMenuItem(hP, IDM_CTX_WINSCP, MF_BYCOMMAND | MF_GRAYED); EnableMenuItem(hP, IDM_CTX_EDIT, MF_BYCOMMAND | MF_GRAYED); EnableMenuItem(hP, IDM_CTX_DELETE, MF_BYCOMMAND | MF_GRAYED); }
                TrackPopupMenu(hP, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hwnd, NULL); DestroyMenu(hM);
            }
        } else if (pH->code == TVN_BEGINDRAGW) {
            LPNMTREEVIEW lp = (LPNMTREEVIEW)lParam;
            if (lp->itemNew.lParam != (LPARAM)-1) { m_hDragItem = lp->itemNew.hItem; TreeView_SelectItem(m_hTreeView, m_hDragItem); SetCapture(m_hwnd); }
        } else if (pH->code == NM_RETURN) {
            TVITEM tvi = { 0 }; tvi.hItem = TreeView_GetSelection(m_hTreeView); tvi.mask = TVIF_PARAM;
            if (TreeView_GetItem(m_hTreeView, &tvi) && tvi.lParam != (LPARAM)-1) {
                size_t idx = (size_t)tvi.lParam;
                if (idx < m_filteredIndices.size()) LaunchSession(m_allConnections[m_filteredIndices[idx]]);
            }
        } else if (pH->code == TVN_KEYDOWN) {
            LPNMTVKEYDOWN pTVK = (LPNMTVKEYDOWN)lParam;
            if (pTVK->wVKey == VK_DELETE) {
                OnDeleteConnection();
            }
        }
    } else if (pH->hwndFrom == m_hTabControl) {
        if (pH->code == TCN_SELCHANGE) OnTabChange();
        else if (pH->code == NM_RCLICK) {
            POINT pt; GetCursorPos(&pt); TCHITTESTINFO ht = { 0 }; ht.pt = pt; ScreenToClient(m_hTabControl, &ht.pt);
            int idx = TabCtrl_HitTest(m_hTabControl, &ht);
            if (idx != -1) {
                TabCtrl_SetCurSel(m_hTabControl, idx); OnTabChange();
                HMENU hM = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TAB_MENU)); TrackPopupMenu(GetSubMenu(hM, 0), TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hwnd, NULL); DestroyMenu(hM);
            }
        }
    }
    return 0;
}

void MainWindow::OnTimer()
{
    std::vector<int> toClose; int count = TabCtrl_GetItemCount(m_hTabControl);
    for (int i = 0; i < count; ++i) {
        TCITEM tie; tie.mask = TCIF_PARAM;
        if (TabCtrl_GetItem(m_hTabControl, i, &tie)) {
            Session* s = (Session*)tie.lParam;
            HANDLE hP = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, s->pid);
            if (hP) { DWORD ec = 0; if (GetExitCodeProcess(hP, &ec) && ec != STILL_ACTIVE) toClose.push_back(i); CloseHandle(hP); }
            else toClose.push_back(i);
        }
    }
    for (int i = (int)toClose.size() - 1; i >= 0; --i) CloseTab(toClose[i]);
}

void MainWindow::OnDestroy()
{
    WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
    if (GetWindowPlacement(m_hwnd, &wp)) {
        ConnectionManager::WindowState state;
        state.x = wp.rcNormalPosition.left; state.y = wp.rcNormalPosition.top;
        state.width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
        state.height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
        state.maximized = (wp.showCmd == SW_MAXIMIZE); state.sidebarWidth = m_treeWidth;
        ConnectionManager::SaveWindowState(state);
    }
    for (Session* s : m_sessions) { HANDLE hP = OpenProcess(PROCESS_TERMINATE, FALSE, s->pid); if (hP) { TerminateProcess(hP, 0); CloseHandle(hP); } }
    PostQuitMessage(0);
}

void MainWindow::OnSize(int width, int height)
{
    if (m_hTreeView && m_hTabControl && m_hStatusBar && m_hSearchEdit) {
        SendMessage(m_hStatusBar, WM_SIZE, 0, 0);
        RECT rcS; GetWindowRect(m_hStatusBar, &rcS);
        int h = height - (rcS.bottom - rcS.top);
        int sh = 22;
        int qh = 24; // Quick bar height
        int currentTreeWidth = m_showSidebar ? m_treeWidth : 0;

        if (m_showSidebar) {
            MoveWindow(m_hSearchEdit, 0, 0, currentTreeWidth, sh, TRUE);
            MoveWindow(m_hTreeView, 0, sh, currentTreeWidth, h - sh, TRUE);
        }
        
        // Quick Bar Layout
        int tabX = currentTreeWidth + (m_showSidebar ? 2 : 0);
        int tabW = width - tabX;
        
        int btnW = 50;
        int chkW = 50;
        int editW = tabW - btnW - chkW - 10;
        
        MoveWindow(m_hQuickEdit, tabX, 0, editW, qh, TRUE);
        MoveWindow(m_hQuickBtn, tabX + editW + 2, 0, btnW, qh, TRUE);
        MoveWindow(m_hQuickChk, tabX + editW + btnW + 6, 0, chkW, qh, TRUE);

        MoveWindow(m_hTabControl, tabX, qh + 2, tabW, h - qh - 2, TRUE);
        OnTabChange();
    }
}