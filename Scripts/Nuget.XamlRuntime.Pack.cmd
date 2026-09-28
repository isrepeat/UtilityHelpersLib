@echo off
setlocal

for %%I in ("%~dp0..") do set "UTILITY_HELPERS_ROOT=%%~fI"
call "%UTILITY_HELPERS_ROOT%\Scripts\Resolve-PackagesFeed.cmd"
if errorlevel 1 endlocal & exit /b %errorlevel%

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%UTILITY_HELPERS_ROOT%\NugetProjects\XamlRuntime\Nuget\XamlRuntime.Package\Nuget.XamlRuntime.Pack.ps1" -UtilityHelpersRoot "%UTILITY_HELPERS_ROOT%" -FeedRoot "%UH_PACKAGES_FEED%" -Configuration Release %*
set "EXIT_CODE=%ERRORLEVEL%"

echo.
echo XamlRuntime Pack finished with exit code %EXIT_CODE%.
pause
endlocal & exit /b %EXIT_CODE%