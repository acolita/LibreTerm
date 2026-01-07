#include "MainWindow.h"
#include <commdlg.h>

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
