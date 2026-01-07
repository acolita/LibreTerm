#include <windows.h>
#include <commctrl.h>

LRESULT CALLBACK EditCtrlSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/)
{
    if (uMsg == WM_KEYDOWN && wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
        SendMessage(hWnd, EM_SETSEL, 0, -1);
        return 0;
    }
    else if (uMsg == WM_NCDESTROY) {
        RemoveWindowSubclass(hWnd, EditCtrlSubclassProc, uIdSubclass);
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK TabCtrlSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/)
{
    if (uMsg == WM_MBUTTONUP || uMsg == WM_LBUTTONDBLCLK) {
        TCHITTESTINFO ht = { 0 };
        ht.pt.x = LOWORD(lParam);
        ht.pt.y = HIWORD(lParam);
        int idx = TabCtrl_HitTest(hWnd, &ht);
        if (idx != -1) {
            HWND hParent = GetParent(hWnd);
            PostMessage(hParent, WM_COMMAND, MAKEWPARAM(idx, 999), (LPARAM)hWnd);
            return 0;
        }
    }
    else if (uMsg == WM_NCDESTROY) {
        RemoveWindowSubclass(hWnd, TabCtrlSubclassProc, uIdSubclass);
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void SubclassEdit(HWND hDlg, int nIDDlgItem) {
    HWND hEdit = GetDlgItem(hDlg, nIDDlgItem);
    if (hEdit) {
        SetWindowSubclass(hEdit, EditCtrlSubclassProc, 0, 0);
    }
}