@ECHO off
SETLOCAL

SET "SCRIPT_DIR=%~dp0"
CALL "%SCRIPT_DIR%Resolve-PackagesFeed.cmd"
IF ERRORLEVEL 1 ENDLOCAL & EXIT /B %ERRORLEVEL%

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%PowerShell\Nuget.CrashHandling.Pack.Release.ps1"
SET "EXIT_CODE=%ERRORLEVEL%"

PAUSE
ENDLOCAL & EXIT /B %EXIT_CODE%