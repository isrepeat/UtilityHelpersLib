@ECHO OFF
SETLOCAL

REM ============================================================================
REM UpdateUtilityHelpersSubmodulesForCurrentDir.bat
REM
REM Copy this .bat to the common directory containing all projects. It scans the
REM directory containing this .bat and its immediate child directories for
REM projects containing UtilityHelpersLib. Clean submodules update automatically.
REM
REM If any submodule is dirty, the engine creates recovery snapshots and imports
REM candidate branches into Integration next to this .bat. The user manually
REM chooses commits, order and messages, resolves conflicts, and pushes the final
REM Integration HEAD. Only then are parent projects updated.
REM ============================================================================

REM Defaults used by most projects.
SET "SUBMODULE_NAME=UtilityHelpersLib"
SET "PROJECT_BRANCH=Last"
SET "SUBMODULE_BRANCH=Last"
SET "SCAN_DEPTH=1"

REM The scan starts beside this copied .bat, not in the console's current folder.
SET "SCAN_ROOT=%~dp0."

REM Full path to the PowerShell engine. Keep this explicit because project and
REM submodule directory names may differ in the future. Spaces are supported.
SET "CORE_SCRIPT=C:\WORK\Projects\Cpp\UtilityHelpersLib\Scripts\PowerShell\UpdateUtilityHelpersSubmodules.ps1"

REM Exact overrides: path^|project branch^|UtilityHelpersLib branch
REM Separate multiple entries with a semicolon. Paths may contain spaces.
SET "PROJECT_OVERRIDES=%~dp0Cpp|Last|Last"

REM Integration is created beside the wrapper as Integration-SUBMODULE_NAME. The
REM engine offers to remove a stale copy and the completed temporary clone.
SET "INTEGRATION_DIR=%~dp0Integration-%SUBMODULE_NAME%"

IF NOT EXIST "%CORE_SCRIPT%" (
    ECHO [UtilityHelpers updater] ERROR: PowerShell engine not found:
    ECHO     %CORE_SCRIPT%
    SET "RESULT=1"
    GOTO :FINISH
)

ECHO [UtilityHelpers updater] Scan root: %SCAN_ROOT%
ECHO [UtilityHelpers updater] Core script: %CORE_SCRIPT%
ECHO [UtilityHelpers updater] Integration: %INTEGRATION_DIR%
ECHO.

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%CORE_SCRIPT%" ^
    -ScanRoot "%SCAN_ROOT%" ^
    -ScanDepth %SCAN_DEPTH% ^
    -ScanProjectBranch "%PROJECT_BRANCH%" ^
    -ScanSubmoduleBranch "%SUBMODULE_BRANCH%" ^
    -ProjectOverrides "%PROJECT_OVERRIDES%" ^
    -SubmoduleName "%SUBMODULE_NAME%" ^
    -IntegrationDirectory "%INTEGRATION_DIR%" ^
    %*
SET "RESULT=%ERRORLEVEL%"

:FINISH
ECHO.
IF NOT "%RESULT%"=="0" (
    ECHO Update failed or was cancelled. Review the message above.
) ELSE (
    ECHO Update completed successfully.
)
PAUSE
ENDLOCAL & EXIT /B %RESULT%

