#include <windows.h>
#include <commctrl.h>

// Add to top of MainWindow.cpp
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

void SubclassEdit(HWND hDlg, int nIDDlgItem) {
    HWND hEdit = GetDlgItem(hDlg, nIDDlgItem);
    if (hEdit) {
        SetWindowSubclass(hEdit, EditCtrlSubclassProc, 0, 0);
    }
}
