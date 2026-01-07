@echo off
taskkill /F /IM LibreTerm.exe 2> nul
taskkill /F /IM putty.exe 2> nul

where cl.exe >nul 2>nul
if %errorlevel% equ 0 (
    echo MSVC environment already set. Skipping vcvarsall.bat.
    goto :build
)

set VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
if not exist %VCVARS% (
    :: Try Community Edition path common on some systems
    set VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
)
if not exist %VCVARS% (
    :: Try Enterprise Edition path
    set VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
)

if not exist %VCVARS% (
    echo Error: Could not find vcvarsall.bat. Please run from Developer Command Prompt or set up VS environment.
    exit /b 1
)

echo Setting up VS environment...
call %VCVARS% x64 > nul

:build
if not exist build mkdir build

echo Compiling Resources...
rc.exe /fo build/LibreTerm.res src/LibreTerm.rc

echo Compiling...
cl.exe /nologo /std:c++17 /EHsc /W4 /O2 /DUNICODE /D_UNICODE src/main.cpp src/MainWindow.cpp src/ProcessUtils.cpp src/ConnectionManager.cpp build/LibreTerm.res /Fe:build/LibreTerm.exe /link user32.lib gdi32.lib comctl32.lib shell32.lib comdlg32.lib

echo Build successful! Output: build/LibreTerm.exe