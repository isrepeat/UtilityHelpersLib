@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\NugetProjects\AndroidBuildTools\Pack.ps1" %*
exit /b %errorlevel%