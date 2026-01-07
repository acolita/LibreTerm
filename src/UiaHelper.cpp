#include <windows.h>
#include <commctrl.h>
#include <uiautomation.h>

// Helper to set AutomationId for WinAppDriver
void SetAutomationId(HWND hwnd, const wchar_t* automationId)
{
    IUIAutomation* pAutomation = NULL;
    if (SUCCEEDED(CoCreateInstance(CLSID_CUIAutomation, NULL, CLSCTX_INPROC_SERVER, IID_IUIAutomation, (void**)&pAutomation)))
    {
        // Setting AutomationId directly on HWND is not standard Win32 API.
        // WinAppDriver usually reads the "Name" or "AutomationId" property exposed by UIA.
        // For standard controls (Edit, Button), the Control ID (GetDlgCtrlID) is often mapped to AutomationId.
        // But for custom windows or main window, we rely on Name.
        
        // However, for explicit AutomationId support without heavy UIA implementation,
        // we can just ensure every control has a unique Control ID (which we do in resource.h)
        // and WinAppDriver will use that as AutomationId.
        
        // For this MVP, we will rely on Control IDs defined in resource.h
        pAutomation->Release();
    }
}
