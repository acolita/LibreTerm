# Building and Running LibreTerm on macOS

This document describes how to cross-compile LibreTerm for Windows on macOS and run it using Wine.

## Prerequisites

- **Homebrew** - https://brew.sh
- **MinGW-w64** - Cross-compiler for Windows executables
- **Wine** - Compatibility layer to run Windows applications

### Install Dependencies

```bash
# Install MinGW-w64 cross-compiler
brew install mingw-w64

# Install Wine (choose one method)
# Method 1: Homebrew (may require Rosetta 2 on Apple Silicon)
brew install --cask wine-stable

# Method 2: Manual download
# Download from https://github.com/Gcenx/macOS_Wine_builds/releases
# Extract to /Applications or /tmp
```

## Building

```bash
# Make the build script executable
chmod +x build-mingw.sh

# Build LibreTerm
./build-mingw.sh
```

The compiled executable will be at `build/LibreTerm.exe`.

## Running

```bash
# Make the run script executable
chmod +x run-wine.sh

# Run LibreTerm under Wine
./run-wine.sh
```

Or run directly:
```bash
wine build/LibreTerm.exe
```

## Known Limitations

When running under Wine on macOS:

| Feature | Status | Notes |
|---------|--------|-------|
| Main UI | Works | Window rendering functional |
| TreeView/Tabs | Works | Common controls work |
| Connection management | Works | INI file storage |
| Credential Manager | Limited | Uses in-memory storage (not persistent) |
| PuTTY embedding | Untested | Requires PuTTY.exe also running in Wine |
| Broadcast mode | Untested | Low-level keyboard hooks may not work |

### Missing Windows Credential Manager

The MinGW build uses a stub implementation for credential storage because
`wincred.h` is not available. Credentials are stored in memory only and will
not persist between sessions. For persistent credential storage, use the
native Windows build.

### PuTTY Requirement

LibreTerm embeds PuTTY windows. To fully test on macOS:
1. Download PuTTY for Windows
2. Place `putty.exe` in Wine's PATH or configure the path in LibreTerm settings
3. Both LibreTerm and PuTTY run under Wine

## Troubleshooting

### "libwinpthread-1.dll not found"
Rebuild with static linking (the default `build-mingw.sh` already does this).

### Wine freezes or wineserver crashes
Wine stability on macOS (especially Apple Silicon) can be problematic. If Wine
hangs or you see "wineserver crashed", try these alternatives:

1. **CrossOver** (commercial, more stable): `brew install --cask crossover`
2. **Windows VM**: Use UTM, Parallels, or VMware Fusion
3. **Test on actual Windows**: Copy `build/LibreTerm.exe` to a Windows machine

### Wine window not appearing
- On Apple Silicon Macs, ensure Rosetta 2 is installed: `softwareupdate --install-rosetta`
- Check if Wine is configured: `wineboot --init`

### Build fails with wincred.h errors
This is expected - the MinGW build uses a stub implementation. The code
conditionally compiles based on `__MINGW32__`.

## Alternative: Native Windows VM

For full functionality, consider running LibreTerm in a Windows VM:
- **Parallels Desktop** (paid)
- **VMware Fusion** (paid/free)
- **UTM** (free, uses QEMU)
