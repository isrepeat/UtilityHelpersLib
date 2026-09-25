@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\..\PackageProjects\Android\AndroidCoreSdk\Package.Android.AndroidCoreSdk.Pack.ps1" -NoPause
set "scriptExitCode=%ERRORLEVEL%"
pause
exit /b %scriptExitCode%