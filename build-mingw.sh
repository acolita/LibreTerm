#!/bin/bash
# Cross-compile LibreTerm for Windows using MinGW-w64 on macOS
# Usage: ./build-mingw.sh

set -e

# MinGW toolchain
CXX="x86_64-w64-mingw32-g++"
WINDRES="x86_64-w64-mingw32-windres"

# Check if toolchain is available
if ! command -v "$CXX" &> /dev/null; then
    echo "Error: MinGW-w64 not found. Install with: brew install mingw-w64"
    exit 1
fi

# Create build directory
mkdir -p build

echo "Compiling Resources..."
$WINDRES src/LibreTerm.rc -O coff -o build/LibreTerm.res

echo "Compiling LibreTerm..."
$CXX \
    -std=c++17 \
    -O2 \
    -Wall -Wextra \
    -DUNICODE -D_UNICODE \
    -static-libgcc -static-libstdc++ -static \
    -mwindows \
    -municode \
    src/main.cpp \
    src/MainWindow.cpp \
    src/MainWindow_Actions.cpp \
    src/MainWindow_Dialogs.cpp \
    src/MainWindow_Session.cpp \
    src/ProcessUtils.cpp \
    src/ConnectionManager.cpp \
    src/CredentialManager.cpp \
    src/SnippetManager.cpp \
    src/Subclass.cpp \
    src/InputHook.cpp \
    build/LibreTerm.res \
    -o build/LibreTerm.exe \
    -luser32 -lgdi32 -lcomctl32 -lshell32 -lcomdlg32 -ladvapi32 -lole32 -luuid

echo "Build successful! Output: build/LibreTerm.exe"
echo ""
echo "To run with Wine:"
echo "  wine build/LibreTerm.exe"
