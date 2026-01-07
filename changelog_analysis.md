# WinSSHTerm Changelog Analysis & Requirements

## Key Learnings & Pitfalls to Avoid
Based on the analysis of WinSSHTerm's history (v1.0 - v2.41), here are critical areas to focus on for `wstc`.

### 1. Window Embedding & Input Focus (CRITICAL)
*   **The Problem:** Embedding `putty.exe` often leads to focus stealing or hotkey swallowing.
*   **Known Bugs:**
    *   "Windows key hotkeys don't trigger when PuTTY has focus" (v2.41.5).
    *   "Alt-Tab switching behavior when PuTTY had focus" (v2.41.3).
    *   "Improved focus when clicking inside PuTTY terminal" (v1.0.4).
*   **Strategy for wstc:** We must ensure the message loop correctly forwards or handles keys when the child window is focused. We might need `AttachThreadInput` or specific keyboard hooks if PuTTY swallows system keys.

### 2. Layout & Display
*   **The Problem:** External windows don't always behave like native child controls.
*   **Known Bugs:**
    *   "Taskbar not hiding in fullscreen mode" (v1.0.5).
    *   "DPI awareness" issues (v2.37.5 explicitly disabled it to prevent layout bugs).
    *   "Regression where Multi-Input menu elements don't always align" (v2.41.4).
*   **Strategy for wstc:**
    *   Handle `WM_SIZE` aggressively.
    *   Consider DPI awareness carefully (maybe disable it initially if PuTTY is not DPI aware).

### 3. Feature Roadmap (Derived from History)

#### Phase 1: The Basics (Matches v1.0 - v1.2)
*   [x] Basic Embedding (Done)
*   [ ] "Clone Connection" (Duplicate session)
*   [ ] Support for KiTTY as an alternative to PuTTY.
*   [ ] "Close all but me" tab management.
*   [ ] Keyboard Shortcuts (AltGr + Space for last tab).

#### Phase 2: Advanced Integration (Matches v2.x)
*   [ ] **Launch Tools:** Architecture to support external tools (RDP, VNC, HTTP) not just SSH.
*   [ ] **Jump Servers:** Tunneling support (Multi-hop).
*   [ ] **File Transfer:** WinSCP integration ("Copy Files").
*   [ ] **X11:** VcXsrv integration.
*   [ ] **Scripts:** "Script Runner" and assigning scripts to buttons.

### 4. Security & Configuration
*   **Notes:**
    *   "Show warnings if an unsecure version of PuTTY/WinSCP is used" (v2.34.1).
    *   Environment variable support for custom tool paths (v2.35.0).
    *   OpenSSH certificate support (v2.37.0).

## Immediate Action Items for MVP
1.  **Focus Management:** Verify if `Alt+Tab` works when the "Fake PuTTY" is focused.
2.  **DPI Check:** Check if the embedded window scales correctly on high DPI screens.
3.  **Path Configuration:** Implement environment variable support or a settings dialog early to avoid hardcoded paths.
