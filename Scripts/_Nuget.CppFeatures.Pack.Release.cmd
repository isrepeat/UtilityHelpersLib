@ECHO off
setlocal EnableExtensions DisableDelayedExpansion
SET "SCRIPT_DIR=%~dp0"
CALL "%SCRIPT_DIR%Resolve-PackagesFeed.cmd"
SET "EXIT_CODE=%ERRORLEVEL%"
IF NOT "%EXIT_CODE%"=="0" GOTO :FeedResolutionFailed
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%PowerShell\Nuget.CppFeatures.Pack.Release.ps1"
SET "EXIT_CODE=%ERRORLEVEL%"

PAUSE
ENDLOCAL & EXIT /B %EXIT_CODE%

:FeedResolutionFailed
ECHO.
ECHO Package feed resolution failed with exit code %EXIT_CODE%.
PAUSE
ENDLOCAL & EXIT /B %EXIT_CODE%