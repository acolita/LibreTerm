@echo off
echo Installing requirements...
pip install -r tests/requirements.txt

echo.
echo Ensuring WinAppDriver is running...
tasklist /FI "IMAGENAME eq WinAppDriver.exe" 2>NUL | find /I /N "WinAppDriver.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo WinAppDriver is already running.
) else (
    echo Starting WinAppDriver...
    start "WinAppDriver" "C:\Program Files\Windows Application Driver\WinAppDriver.exe"
    timeout /t 3
)

echo.
echo Running Tests...
pytest tests/ -v
