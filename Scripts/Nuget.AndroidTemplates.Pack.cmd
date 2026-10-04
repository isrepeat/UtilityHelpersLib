@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0..") do set "UTILITY_HELPERS_ROOT=%%~fI"
call "%UTILITY_HELPERS_ROOT%\Scripts\Resolve-PackagesFeed.cmd"
set "EXIT_CODE=%ERRORLEVEL%"
if not "%EXIT_CODE%"=="0" goto :FeedResolutionFailed

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%UTILITY_HELPERS_ROOT%\NugetProjects\AndroidTemplates\Pack.ps1" -FeedPath "%UH_PACKAGES_FEED%" %*
set "EXIT_CODE=%ERRORLEVEL%"

echo.
echo AndroidTemplates Pack finished with exit code %EXIT_CODE%.
pause
endlocal & exit /b %EXIT_CODE%

:FeedResolutionFailed
echo.
echo AndroidTemplates Pack failed with exit code %EXIT_CODE%.
pause
endlocal & exit /b %EXIT_CODE%