# wstc Architecture & Design Plan (C++ Win32)

## 1. Architectural Overview
The application will follow a **Event-Driven Object-Oriented** architecture, wrapping raw Win32 C-style APIs into C++ classes to manage state and logic cleanly.

### High-Level Components
1.  **`App` (Singleton/Context):** Manages the application lifecycle, command line args, and global resources (fonts, icons).
2.  **`MainWindow` (View/Controller):** Represents the primary application window. It owns the child controls (TreeView, TabControl) and orchestrates the layout. It handles the Windows Message Loop (`WndProc`) for the main window.
3.  **`SessionManager` (Model/Controller):** Manages the lifecycle of active SSH sessions. It links a specific Tab Index to a specific PuTTY Process ID (PID) and Window Handle (HWND).
4.  **`ProcessEmbedder` (Service):** A specialized utility responsible for the low-level "magic" of spawning processes, locating their windows, and performing the Win32 `SetParent` injection.
5.  **`ConnectionStore` (Model):** Responsible for loading and saving connection definitions (Name, IP, Port, User) from disk.

## 2. Class Design & Responsibilities

### A. `WindowBase` (Abstract Class)
*   **Purpose:** Boilerplate wrapper for creating a Win32 window and routing the static `WndProc` to a member function `HandleMessage`.
*   **Methods:** `Create()`, `Show()`, `HandleMessage(UINT, WPARAM, LPARAM)`.

### B. `MainWindow : public WindowBase`
*   **Members:**
    *   `HWND hTreeView`: Handle to the connections list.
    *   `HWND hTabControl`: Handle to the tabs.
    *   `SessionManager sessionMgr`: Composition.
*   **Key Operations:**
    *   `OnCreate()`: Initialize common controls (`InitCommonControlsEx`), create TreeView and TabControl.
    *   `OnResize()`: Calculate layout logic (TreeView takes left 20%, TabControl takes right 80%). Call `MoveWindow` on child controls.
    *   `OnTreeDoubleClick()`: callback -> `sessionMgr.CreateSession(connectionInfo)`.
    *   `OnTabChanged()`: callback -> `sessionMgr.SwitchToSession(tabIndex)`.
    *   `OnClose()`: Clean up all processes.

### C. `SessionManager`
*   **Structure:** `struct Session { DWORD pid; HWND hwnd; int tabIndex; }`
*   **Members:** `std::vector<Session> sessions;`
*   **Operations:**
    *   `StartSession(Connection info, HWND hParentTab)`:
        1.  Format command line for PuTTY.
        2.  Call `ProcessEmbedder::LaunchAndEmbed`.
        3.  Store the result in `sessions`.
    *   `ResizeCurrentSession(RECT targetRect)`: Resizes the currently active embedded window to fit the tab display area.

### D. `ProcessEmbedder` (Static Helper / Namespace)
*   **Operations:**
    *   `LaunchProcess(wstring cmdLine) -> DWORD pid`: Uses `CreateProcess`.
    *   `FindWindowForProcess(DWORD pid) -> HWND`: Uses `EnumWindows` and `GetWindowThreadProcessId` to find the main window of the spawned process. **Critical:** Needs a retry loop/timeout because the window isn't created instantly.
    *   `Embed(HWND child, HWND parent)`:
        *   `ShowWindow(child, SW_HIDE)` (prevent flicker).
        *   `SetParent(child, parent)`.
        *   `SetWindowLong(child, GWL_STYLE, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS ...)` (Remove borders/caption).
        *   `MoveWindow(...)`.

## 3. Data Flow

### Scenario: User Starts a Connection
1.  **User Event:** Double-click item in `MainWindow` TreeView.
2.  **Controller Action:** `MainWindow` extracts `ConnectionInfo` from the tree item.
3.  **Logic:** `MainWindow` calls `SessionManager::StartSession(info)`.
4.  **Process:** `SessionManager` constructs `putty.exe -ssh ...` string.
5.  **Process:** `ProcessEmbedder` spawns `putty.exe`.
6.  **Wait:** `ProcessEmbedder` polls for the new HWND.
7.  **Embed:** Once found, `ProcessEmbedder` strips the border and calls `SetParent` to the Main Window (or a specific container HWND).
8.  **UI Update:** `MainWindow` adds a new Tab to `hTabControl`.
9.  **Layout:** `MainWindow` triggers a resize to ensure the new PuTTY window fills the tab area.

## 4. Technology specifics (Win32)
*   **String Handling:** `std::wstring` (Unicode `CreateProcessW`, `CreateWindowExW`).
*   **Resource Management:** RAII for Handles (smart pointers or custom `Handle` wrappers) to ensure `CloseHandle` is called.
*   **Compiling:** MSVC (`cl.exe`), linking against `user32.lib`, `gdi32.lib`, `comctl32.lib`.

## 5. MVP Implementation Plan (Files)
1.  `src/main.cpp`: Entry point.
2.  `src/Window.h/cpp`: Base window class.
3.  `src/MainWindow.h/cpp`: Main application logic.
4.  `src/ProcessUtils.h/cpp`: Embedding logic.
5.  `wstc.vcxproj`: Project definition.
