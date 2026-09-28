@echo off
setlocal EnableExtensions DisableDelayedExpansion

for %%I in ("%~dp0..\..") do set "UTILITY_HELPERS_ROOT=%%~fI"
call "%UTILITY_HELPERS_ROOT%\Scripts\Resolve-PackagesFeed.cmd"
set "EXIT_CODE=%ERRORLEVEL%"
if not "%EXIT_CODE%"=="0" goto :FeedResolutionFailed

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%UTILITY_HELPERS_ROOT%\PackageProjects\Android\AndroidAppKit\Pack.ps1" -PackagesFeedPath "%UH_PACKAGES_FEED%" -NoPause %*
set "EXIT_CODE=%ERRORLEVEL%"

echo.
echo AndroidAppKit Pack finished with exit code %EXIT_CODE%.
pause
endlocal & exit /b %EXIT_CODE%

:FeedResolutionFailed
echo.
echo Package feed resolution failed with exit code %EXIT_CODE%.
pause
endlocal & exit /b %EXIT_CODE%