@echo off
call "%~dp0..\Resolve-PackagesFeed.cmd"
if errorlevel 1 exit /b %errorlevel%

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\..\PackageProjects\Android\AndroidAppKit\Package.Android.AndroidAppKit.Pack.ps1" -PackagesFeedPath "%UH_PACKAGES_FEED%" -NoPause
set "scriptExitCode=%ERRORLEVEL%"
pause
exit /b %scriptExitCode%