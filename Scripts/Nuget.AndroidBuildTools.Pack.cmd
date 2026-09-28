@echo off
call "%~dp0Resolve-PackagesFeed.cmd"
if errorlevel 1 exit /b %errorlevel%

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\NugetProjects\AndroidBuildTools\Pack.ps1" -FeedPath "%UH_PACKAGES_FEED%" %*
exit /b %errorlevel%