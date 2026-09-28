@echo off
setlocal EnableExtensions DisableDelayedExpansion
for %%I in ("%~dp0..") do set "UTILITY_HELPERS_ROOT=%%~fI"
call "%UTILITY_HELPERS_ROOT%\Scripts\Resolve-PackagesFeed.cmd"
set "EXIT_CODE=%ERRORLEVEL%"
if not "%EXIT_CODE%"=="0" goto :FeedResolutionFailed
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%UTILITY_HELPERS_ROOT%\NugetProjects\AndroidAppPreviewer.PluginSDK\Nuget\AndroidAppPreviewer.PluginSDK.Package\Pack.ps1" -FeedRoot "%UH_PACKAGES_FEED%" %*
set "EXIT_CODE=%ERRORLEVEL%"

echo.
echo AndroidAppPreviewer.PluginSDK Pack finished with exit code %EXIT_CODE%.
pause
endlocal & exit /b %EXIT_CODE%

:FeedResolutionFailed
echo.
echo Package feed resolution failed with exit code %EXIT_CODE%.
pause
endlocal & exit /b %EXIT_CODE%