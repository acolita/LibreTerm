@echo off
taskkill /F /IM LibreTerm.exe 2> nul
taskkill /F /IM putty.exe 2> nul

set VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"

if not exist %VCVARS% (
    echo Error: Could not find vcvarsall.bat at %VCVARS%
    exit /b 1
)

echo Setting up VS environment...
call %VCVARS% x64 > nul

if not exist build mkdir build

echo Compiling Resources...
rc.exe /fo build/LibreTerm.res src/LibreTerm.rc

echo Compiling...
cl.exe /nologo /std:c++17 /EHsc /W4 /O2 /DUNICODE /D_UNICODE src/main.cpp src/MainWindow.cpp src/ProcessUtils.cpp src/ConnectionManager.cpp build/LibreTerm.res /Fe:build/LibreTerm.exe /link user32.lib gdi32.lib comctl32.lib shell32.lib comdlg32.lib

echo Build successful! Output: build/LibreTerm.exe