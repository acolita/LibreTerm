#!/bin/bash
# Run LibreTerm under Wine on macOS
# Usage: ./run-wine.sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
EXE_PATH="$SCRIPT_DIR/build/LibreTerm.exe"

# Check if executable exists
if [ ! -f "$EXE_PATH" ]; then
    echo "Error: LibreTerm.exe not found. Run ./build-mingw.sh first."
    exit 1
fi

# Try to find Wine
WINE=""
if [ -d "/tmp/Wine Stable.app" ]; then
    WINE="/tmp/Wine Stable.app/Contents/Resources/wine/bin/wine"
elif [ -d "/Applications/Wine Stable.app" ]; then
    WINE="/Applications/Wine Stable.app/Contents/Resources/wine/bin/wine"
elif command -v wine64 &> /dev/null; then
    WINE="wine64"
elif command -v wine &> /dev/null; then
    WINE="wine"
fi

if [ -z "$WINE" ] || [ ! -x "$WINE" ]; then
    echo "Error: Wine not found."
    echo ""
    echo "Install Wine with one of these methods:"
    echo "  1. brew install --cask wine-stable"
    echo "  2. Download from https://github.com/Gcenx/macOS_Wine_builds/releases"
    echo ""
    exit 1
fi

echo "Running LibreTerm with Wine..."
echo "Wine: $WINE"
echo ""

# Run Wine
"$WINE" "$EXE_PATH" 2>&1
