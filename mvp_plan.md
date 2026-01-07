# wstc v0.0.1 MVP Plan (Native C++ Win32) - PROGRESS: 80%

**Goal:** Create a high-performance, lightweight native Windows application (C++ / Win32 API) that functions as a clone of WinSSHTerm.

## Status Update
- [x] Win32 Skeleton with Message Loop.
- [x] TreeView and TabControl integration.
- [x] Process Spawning and Window Swallowing (Embedding).
- [x] **Multi-session support** (Multiple tabs, each with its own process).
- [x] **Process Lifecycle Management** (Cleaning up children on exit).
- [x] **Basic Menu Bar** (File, Help).
- [x] **Fake PuTTY** for testing without installation.

## Remaining for v0.0.1
1. [ ] **Persistence:** Save/Load connection list from JSON.
2. [ ] **Splitter:** Add draggable vertical splitter for sidebar.
3. [ ] **Connection Dialog:** Form to add new hosts (not just hardcoded).
4. [ ] **Refinement:** Handle child process exit (closing tab automatically).

## Architecture
- `MainWindow`: Event-driven controller.
- `ProcessUtils`: Native Win32 embedding engine.
- `Session`: Data structure linking PID/HWND/TabIndex.
