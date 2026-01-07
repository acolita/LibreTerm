#include "ProcessUtils.h"
#include <thread>
#include <chrono>

namespace ProcessUtils
{
    DWORD LaunchProcess(const std::wstring& cmdLine)
    {
        STARTUPINFO si = { sizeof(STARTUPINFO) };
        PROCESS_INFORMATION pi = { 0 };

        // CreateProcess requires a mutable string
        std::wstring mutableCmd = cmdLine;

        BOOL success = CreateProcess(
            NULL, 
            &mutableCmd[0],
            NULL, NULL, FALSE, 0, NULL, NULL, 
            &si, &pi
        );

        if (!success) return 0;

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess); // We only need the PID, so we can close the handle.
        return pi.dwProcessId;
    }

    struct EnumData {
        DWORD targetPid;
        HWND resultHwnd;
    };

    BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
    {
        EnumData* data = (EnumData*)lParam;
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);

        if (pid == data->targetPid)
        {
            // Found a window for this process.
            // Check if it's visible to avoid grabbing hidden initial windows
            if (IsWindowVisible(hwnd))
            {
                data->resultHwnd = hwnd;
                return FALSE; // Stop enumeration
            }
        }
        return TRUE; // Continue
    }

    HWND WaitForWindow(DWORD pid, int timeoutMs)
    {
        EnumData data = { pid, NULL };
        int elapsed = 0;
        int interval = 100;

        while (elapsed < timeoutMs)
        {
            EnumWindows(EnumWindowsProc, (LPARAM)&data);
            if (data.resultHwnd != NULL)
            {
                return data.resultHwnd;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            elapsed += interval;
        }

        return NULL;
    }

    void EmbedWindow(HWND child, HWND parent)
    {
        // 1. Hide first to avoid visual glitches during style change
        ShowWindow(child, SW_HIDE);

        // Attach input threads to ensure focus works
        DWORD childThread = GetWindowThreadProcessId(child, NULL);
        DWORD parentThread = GetCurrentThreadId();
        if (childThread != parentThread) {
            AttachThreadInput(parentThread, childThread, TRUE);
        }

        // 2. Change style: Remove Popup/Caption, make Child
        LONG_PTR style = GetWindowLongPtr(child, GWL_STYLE);
        style &= ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
        style |= WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | WS_TABSTOP;
        SetWindowLongPtr(child, GWL_STYLE, style);

        // Remove extended styles that might interfere (like taskbar item)
        LONG_PTR exStyle = GetWindowLongPtr(child, GWL_EXSTYLE);
        exStyle &= ~(WS_EX_APPWINDOW | WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
        SetWindowLongPtr(child, GWL_EXSTYLE, exStyle);

        // 3. Set Parent
        SetParent(child, parent);

        // 4. Force styles update and Z-order
        SetWindowPos(child, HWND_TOP, 0, 0, 0, 0, 
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            
        // Detach input threads
        if (childThread != parentThread) {
            AttachThreadInput(parentThread, childThread, FALSE);
        }
    }
}
