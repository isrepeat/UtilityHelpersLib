@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\..\PackageProjects\Android\AndroidAppKit\Package.Android.AndroidAppKit.Pack.ps1" -NoPause
set "scriptExitCode=%ERRORLEVEL%"
pause
exit /b %scriptExitCode%