# LibreTerm

**LibreTerm** is a high-performance, open-source tabbed SSH solution for Windows. It acts as a powerful wrapper for PuTTY and WinSCP, providing a centralized and organized environment for remote server management.

## Key Features

- **Native Performance**: Written in pure C++ using the Win32 API. No heavy frameworks, no Electron. Just raw speed and minimal memory footprint.
- **Window Embedding**: Seamlessly embeds PuTTY windows directly into tabs for a unified workspace.
- **Credential Manager (v1.0.0)**: Create reusable authentication profiles (Alias -> User/Pass/Key). Share one credential across multiple connections.
- **Advanced Tab Management (v1.1.0)**: Rename tabs on the fly, close tabs to the right, or close all other tabs.
- **Snippet Manager (v1.2.0)**: Store and send reusable command snippets to active or all sessions.
- **Quick Command Bar (v1.2.0)**: Type and send commands instantly without leaving the main window.
- **Automated Host Key Acceptance (v1.2.0)**: PuTTY and WinSCP now automatically handle unknown SSH host key warnings.
- **View Modes (v1.1.0)**: 
    - **Fullscreen (`F11`)**: Immerse in a borderless terminal experience.
    - **Focus Mode (`Ctrl+B`)**: Toggle the sidebar to maximize terminal width.
- **Organization**: Group connections into folders with a real-time searchable sidebar.
- **WinSCP Integration**: One-click file transfer for any active connection.
- **Productivity Shortcuts**:
  - `Ctrl + Tab` / `Ctrl + Shift + Tab`: Switch between terminal sessions.
  - `Ctrl + W`: Close the active session.
  - `Ctrl + Q`: Quick Connect without saving to list.
  - `Ctrl + A`: Select all in any input field.
- **Draggable Splitter**: Easily resize the sidebar to fit your workspace.
- **State Persistence**: Remembers window size, position, maximized state, and sidebar width between sessions.
- **CI/CD Integrated**: Automated builds and releases via GitHub Actions.
- **Automated QA**: Comprehensive E2E test suite using Microsoft WinAppDriver.

## Prerequisites

- **PuTTY**: Ensure `putty.exe` is in your system `PATH` or configure the path in `File -> Settings`.
- **WinSCP** (Optional): For file transfer support.

## Building from Source

1.  Install **Visual Studio 2022** with the "Desktop development with C++" workload.
2.  Clone the repository.
3.  Run `build.bat`.
4.  The compiled executable will be located in `build/LibreTerm.exe`.

## Running E2E Tests

1.  Ensure [WinAppDriver](https://github.com/microsoft/WinAppDriver) is installed.
2.  Ensure Python 3.x is installed.
3.  Run `run_tests.bat`. This will install dependencies and execute the test suite.

## License

MIT
