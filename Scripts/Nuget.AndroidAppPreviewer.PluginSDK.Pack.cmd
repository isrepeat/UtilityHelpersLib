@echo off
setlocal

for %%I in ("%~dp0..") do set "UTILITY_HELPERS_ROOT=%%~fI"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%UTILITY_HELPERS_ROOT%\NugetProjects\AndroidAppPreviewer\Nuget\AndroidAppPreviewer.PluginSDK.Package\Nuget.AndroidAppPreviewer.PluginSDK.Pack.ps1" %*
set "EXIT_CODE=%ERRORLEVEL%"

echo.
echo AndroidAppPreviewer.PluginSDK Pack finished with exit code %EXIT_CODE%.
pause
endlocal & exit /b %EXIT_CODE%