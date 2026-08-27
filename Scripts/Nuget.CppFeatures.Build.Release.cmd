@ECHO off
SETLOCAL

SET "SCRIPT_DIR=%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%PowerShell\Nuget.CppFeatures.Build.Release.ps1"
SET "EXIT_CODE=%ERRORLEVEL%"

PAUSE
ENDLOCAL & EXIT /B %EXIT_CODE%
