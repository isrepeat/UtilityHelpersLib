@ECHO off
SETLOCAL

SET SCRIPT_DIR=%~dp0
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%\Utility.InjectHelpersToSolution.ps1"
PAUSE

ENDLOCAL