#include <windows.h>
#include <commctrl.h>
#include "MainWindow.h" // Needed for Session struct

// Global hook handle
HHOOK g_hKeyboardHook = NULL;
HWND g_hMainWindow = NULL;

// External reference to sessions from MainWindow (we need a way to access them)
// For MVP, we'll store a pointer to MainWindow instance
MainWindow* g_pMainWindow = NULL;

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        // Handle KeyDown and KeyUp
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN || wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
        {
            // Check if broadcast is active
            if (g_pMainWindow && g_pMainWindow->IsBroadcastActive())
            {
                KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
                
                // Reconstruct lParam for PostMessage
                LPARAM newLParam = 1; // Repeat count
                newLParam |= ((LPARAM)pKey->scanCode << 16);
                if (pKey->flags & LLKHF_EXTENDED) newLParam |= ((LPARAM)1 << 24);
                if (pKey->flags & LLKHF_ALTDOWN)  newLParam |= ((LPARAM)1 << 29);
                if (pKey->flags & LLKHF_UP)       newLParam |= ((LPARAM)1 << 31);
                // Bit 30: Previous key state. Hard to know exactly without tracking, 
                // but 0 is usually fine for KeyDown (was up), 1 for KeyUp.
                // For broadcasting typing, it's usually fine.

                // Determine if we are the source
                if (g_pMainWindow->IsSourceOfFocus())
                {
                    g_pMainWindow->BroadcastKey((UINT)wParam, pKey->vkCode, newLParam);
                }
            }
        }
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}
