#pragma once
#include <windows.h>
#include <string>

namespace ProcessUtils
{
    // Launches a process and returns its Process ID.
    // returns 0 on failure.
    DWORD LaunchProcess(const std::wstring& cmdLine);

    // Waits for a main window belonging to the given PID to appear.
    // Retries for 'timeoutMs' milliseconds.
    // Returns NULL if not found.
    HWND WaitForWindow(DWORD pid, int timeoutMs = 5000);

    // Strips the borders/caption from 'child' and reparents it to 'parent'.
    // Also sets the style to WS_CHILD.
    void EmbedWindow(HWND child, HWND parent);
}
