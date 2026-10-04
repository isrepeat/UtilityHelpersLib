@echo off

if not defined UH_PACKAGES_FEED (
    set /p "UH_PACKAGES_FEED=Packages feed path: "
)

if not defined UH_PACKAGES_FEED (
    echo Packages feed path is required. Set UH_PACKAGES_FEED or enter the path.
    exit /b 1
)

for %%I in ("%UH_PACKAGES_FEED%") do set "UH_PACKAGES_FEED=%%~fI"