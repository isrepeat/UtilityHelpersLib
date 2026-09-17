@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0PowerShell\Install-TrailingWhitespaceWatcher.ps1"
set "exit_code=%errorlevel%"
if not "%exit_code%"=="0" (
    echo Failed to install or start the trailing whitespace watcher.
) else (
    echo The trailing whitespace watcher is installed and running.
)
pause
exit /b %exit_code%