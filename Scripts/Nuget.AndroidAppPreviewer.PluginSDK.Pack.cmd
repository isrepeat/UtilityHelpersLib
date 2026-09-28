@echo off
setlocal

for %%I in ("%~dp0..") do set "UTILITY_HELPERS_ROOT=%%~fI"
call "%UTILITY_HELPERS_ROOT%\Scripts\Resolve-PackagesFeed.cmd"
if errorlevel 1 endlocal & exit /b %errorlevel%

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%UTILITY_HELPERS_ROOT%\NugetProjects\AndroidAppPreviewer\Nuget\AndroidAppPreviewer.PluginSDK.Package\Nuget.AndroidAppPreviewer.PluginSDK.Pack.ps1" -FeedRoot "%UH_PACKAGES_FEED%" %*
set "EXIT_CODE=%ERRORLEVEL%"

echo.
echo AndroidAppPreviewer.PluginSDK Pack finished with exit code %EXIT_CODE%.
pause
endlocal & exit /b %EXIT_CODE%