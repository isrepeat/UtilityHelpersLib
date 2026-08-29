@echo off
set NUGETFILE=CppFeatures.1.0.3.nupkg

set PATH=%PATH%;%~dp0

setlocal
:PROMPT
SET /P AREYOUSURE=Upload %NUGETFILE% (Y/[N])?
IF /I "%AREYOUSURE%" NEQ "Y" GOTO END

echo upload nuget ...
dotnet nuget push --source http://nuget.dct.ua/v3/index.json Nuget\%NUGETFILE%

:END
endlocal

pause
