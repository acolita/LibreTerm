# LibreTerm

**LibreTerm** is an open-source clone of WinSSHTerm.

## Goal
To provide a free, open-source alternative to WinSSHTerm with similar functionality for managing SSH connections and terminal sessions on Windows.

## Features
- **Native Win32 C++ Application**: Fast, lightweight, and low memory usage.
- **Multi-Tab Interface**: Manage multiple SSH sessions in tabs.
- **PuTTY Integration**: Embeds PuTTY windows directly into tabs.
- **WinSCP Integration**: Launch file transfer sessions with a single click.
- **Connection Manager**: Organize connections into folders.
- **Password Support**: Save passwords (plain text) for auto-login.
- **Keyboard Shortcuts**: `Ctrl+Tab`, `Ctrl+W`, etc.
- **Portable**: Can be configured to store settings locally (currently %APPDATA%).

## Building
1.  Ensure Visual Studio 2022 (with C++ workload) is installed.
2.  Run `build.bat`.
3.  The executable will be in `build/LibreTerm.exe`.

## License
MIT