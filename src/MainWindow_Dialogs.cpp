#include "MainWindow.h"
#include "CredentialManager.h"
#include <commdlg.h>

// ...
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
        
        // Populate Credential Combo
        {
            auto creds = CredentialManager::LoadCredentials();
            SendDlgItemMessage(hDlg, IDC_COMBO_CREDENTIAL, CB_ADDSTRING, 0, (LPARAM)L"<None>");
            for (const auto& c : creds) {
                SendDlgItemMessage(hDlg, IDC_COMBO_CREDENTIAL, CB_ADDSTRING, 0, (LPARAM)c.alias.c_str());
            }
            
            // Select current
            int index = 0;
            if (!pConn->credentialAlias.empty()) {
                int found = (int)SendDlgItemMessage(hDlg, IDC_COMBO_CREDENTIAL, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pConn->credentialAlias.c_str());
                if (found != CB_ERR) index = found;
            }
            SendDlgItemMessage(hDlg, IDC_COMBO_CREDENTIAL, CB_SETCURSEL, index, 0);
        }

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
            
            // Handle Credential Selection
            int sel = (int)SendDlgItemMessage(hDlg, IDC_COMBO_CREDENTIAL, CB_GETCURSEL, 0, 0);
            if (sel > 0) { // 0 is <None>
                wchar_t alias[256];
                SendDlgItemMessage(hDlg, IDC_COMBO_CREDENTIAL, CB_GETLBTEXT, sel, (LPARAM)alias);
                pConn->credentialAlias = alias;
                
                // If using credential, we can optionally clear user/pass or overwrite them
                // For this implementation, we overwrite them with the credential values to ensure immediate consistency
                Credential c = CredentialManager::GetCredential(alias);
                pConn->user = c.username;
                pConn->password = c.password;
            } else {
                pConn->credentialAlias = L""; // Clear alias
                GetDlgItemText(hDlg, IDC_EDIT_USER, buf, 256); pConn->user = buf;
                GetDlgItemText(hDlg, IDC_EDIT_PASSWORD, buf, 256); pConn->password = buf;
            }

            GetDlgItemText(hDlg, IDC_EDIT_ARGS, buf, 256); pConn->args = buf;
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDC_COMBO_CREDENTIAL && HIWORD(wParam) == CBN_SELCHANGE) {
            int sel = (int)SendDlgItemMessage(hDlg, IDC_COMBO_CREDENTIAL, CB_GETCURSEL, 0, 0);
            if (sel > 0) {
                wchar_t alias[256];
                SendDlgItemMessage(hDlg, IDC_COMBO_CREDENTIAL, CB_GETLBTEXT, sel, (LPARAM)alias);
                Credential c = CredentialManager::GetCredential(alias);
                SetDlgItemText(hDlg, IDC_EDIT_USER, c.username.c_str());
                SetDlgItemText(hDlg, IDC_EDIT_PASSWORD, c.password.c_str());
            } else {
                // If None selected, maybe clear fields? Or leave as is.
                SetDlgItemText(hDlg, IDC_EDIT_USER, L"");
                SetDlgItemText(hDlg, IDC_EDIT_PASSWORD, L"");
            }
        }
        else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK MainWindow::CredentialManagerDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
    static std::vector<Credential> creds;
    switch (message) {
    case WM_INITDIALOG:
        creds = CredentialManager::LoadCredentials();
        for (const auto& c : creds) {
            SendDlgItemMessage(hDlg, IDC_LIST_CREDENTIALS, LB_ADDSTRING, 0, (LPARAM)c.alias.c_str());
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_CRED_ADD:
            {
                Credential c;
                if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CREDENTIAL_EDIT), hDlg, CredentialEditDialogProc, (LPARAM)&c) == IDOK) {
                    creds.push_back(c);
                    CredentialManager::SaveCredentials(creds);
                    int idx = (int)SendDlgItemMessage(hDlg, IDC_LIST_CREDENTIALS, LB_ADDSTRING, 0, (LPARAM)c.alias.c_str());
                    SendDlgItemMessage(hDlg, IDC_LIST_CREDENTIALS, LB_SETCURSEL, idx, 0);
                }
            }
            break;
        case IDC_BTN_CRED_EDIT:
            {
                int sel = (int)SendDlgItemMessage(hDlg, IDC_LIST_CREDENTIALS, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR) {
                    wchar_t buf[256];
                    SendDlgItemMessage(hDlg, IDC_LIST_CREDENTIALS, LB_GETTEXT, sel, (LPARAM)buf);
                    for (auto& c : creds) {
                        if (c.alias == buf) {
                            if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CREDENTIAL_EDIT), hDlg, CredentialEditDialogProc, (LPARAM)&c) == IDOK) {
                                CredentialManager::SaveCredentials(creds);
                                // Update list text just in case alias changed
                                // For MVP, simple remove and add
                                SendDlgItemMessage(hDlg, IDC_LIST_CREDENTIALS, LB_DELETESTRING, sel, 0);
                                int newIdx = (int)SendDlgItemMessage(hDlg, IDC_LIST_CREDENTIALS, LB_ADDSTRING, 0, (LPARAM)c.alias.c_str());
                                SendDlgItemMessage(hDlg, IDC_LIST_CREDENTIALS, LB_SETCURSEL, newIdx, 0);
                            }
                            break;
                        }
                    }
                }
            }
            break;
        case IDC_BTN_CRED_DELETE:
            {
                int sel = (int)SendDlgItemMessage(hDlg, IDC_LIST_CREDENTIALS, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR) {
                    wchar_t buf[256];
                    SendDlgItemMessage(hDlg, IDC_LIST_CREDENTIALS, LB_GETTEXT, sel, (LPARAM)buf);
                    for (auto it = creds.begin(); it != creds.end(); ++it) {
                        if (it->alias == buf) {
                            creds.erase(it);
                            CredentialManager::SaveCredentials(creds);
                            SendDlgItemMessage(hDlg, IDC_LIST_CREDENTIALS, LB_DELETESTRING, sel, 0);
                            break;
                        }
                    }
                }
            }
            break;
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK MainWindow::CredentialEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static Credential* pCred = NULL;
    switch (message) {
    case WM_INITDIALOG:
        pCred = (Credential*)lParam;
        SetDlgItemText(hDlg, IDC_EDIT_CRED_ALIAS, pCred->alias.c_str());
        SetDlgItemText(hDlg, IDC_EDIT_CRED_USER, pCred->username.c_str());
        SetDlgItemText(hDlg, IDC_EDIT_CRED_PASS, pCred->password.c_str());
        SetDlgItemText(hDlg, IDC_EDIT_CRED_KEY, pCred->keyPath.c_str());
        
        SubclassEdit(hDlg, IDC_EDIT_CRED_ALIAS);
        SubclassEdit(hDlg, IDC_EDIT_CRED_USER);
        SubclassEdit(hDlg, IDC_EDIT_CRED_PASS);
        SubclassEdit(hDlg, IDC_EDIT_CRED_KEY);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            wchar_t buf[256];
            GetDlgItemText(hDlg, IDC_EDIT_CRED_ALIAS, buf, 256); pCred->alias = buf;
            GetDlgItemText(hDlg, IDC_EDIT_CRED_USER, buf, 256); pCred->username = buf;
            GetDlgItemText(hDlg, IDC_EDIT_CRED_PASS, buf, 256); pCred->password = buf;
            GetDlgItemText(hDlg, IDC_EDIT_CRED_KEY, buf, 256); pCred->keyPath = buf;
            if (pCred->alias.empty()) {
                MessageBox(hDlg, L"Alias cannot be empty", L"Error", MB_OK);
                return (INT_PTR)TRUE;
            }
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDC_BTN_CRED_BROWSE) {
            OPENFILENAME ofn; wchar_t szF[MAX_PATH] = { 0 }; ZeroMemory(&ofn, sizeof(ofn)); ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hDlg; ofn.lpstrFile = szF; ofn.nMaxFile = sizeof(szF); ofn.lpstrFilter = L"Key Files\0*.ppk;*.pem\0All Files\0*.*\0"; ofn.nFilterIndex = 1; ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            if (GetOpenFileName(&ofn)) SetDlgItemText(hDlg, IDC_EDIT_CRED_KEY, ofn.lpstrFile);
        }
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
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDC_BUTTON_BROWSE || LOWORD(wParam) == IDC_BUTTON_BROWSE_WINSCP) {
            OPENFILENAME ofn;
            wchar_t szFile[MAX_PATH] = { 0 };
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hDlg;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = L"Executable Files\0*.exe\0All Files\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn))
            {
                SetDlgItemText(hDlg, (LOWORD(wParam) == IDC_BUTTON_BROWSE) ? IDC_EDIT_PUTTY_PATH : IDC_EDIT_WINSCP_PATH, ofn.lpstrFile);
            }
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK MainWindow::QuickConnectDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static Connection* pConn = NULL;
    switch (message) {
    case WM_INITDIALOG:
        pConn = (Connection*)lParam;
        SetDlgItemText(hDlg, IDC_EDIT_HOST, pConn->host.c_str());
        SetDlgItemText(hDlg, IDC_EDIT_PORT, pConn->port.c_str());
        SetDlgItemText(hDlg, IDC_EDIT_USER, pConn->user.c_str());
        SetDlgItemText(hDlg, IDC_EDIT_PASSWORD, pConn->password.c_str());
        
        SubclassEdit(hDlg, IDC_EDIT_HOST);
        SubclassEdit(hDlg, IDC_EDIT_PORT);
        SubclassEdit(hDlg, IDC_EDIT_USER);
        SubclassEdit(hDlg, IDC_EDIT_PASSWORD);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            wchar_t buf[256];
            GetDlgItemText(hDlg, IDC_EDIT_HOST, buf, 256); pConn->host = buf;
            GetDlgItemText(hDlg, IDC_EDIT_PORT, buf, 256); pConn->port = buf;
            GetDlgItemText(hDlg, IDC_EDIT_USER, buf, 256); pConn->user = buf;
            GetDlgItemText(hDlg, IDC_EDIT_PASSWORD, buf, 256); pConn->password = buf;
            pConn->name = L"Quick: " + pConn->host;
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
