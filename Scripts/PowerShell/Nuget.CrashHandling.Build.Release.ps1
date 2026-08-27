$ErrorActionPreference = 'Stop'

$solutionScriptPath = Join-Path $PSScriptRoot 'Solution.UtilityHelpersLib.Nugets.ps1'
$solutionTargets = @(
    'NugetProjects\CrashHandling\Nuget\CrashHandling.Desktop',
    'NugetProjects\CrashHandling\Nuget\CrashHandling.WinRt.WRC',
    'NugetProjects\CrashHandling\Nuget\CrashHandling.WinRt.Projection'
)

# TODO: Переписать логику NuGet-пакета CrashHandling. MinidumpWriter сейчас
#       исключён из решения и сценария сборки, поскольку требует Qt MSBuild и Qt SDK.

foreach ($solutionTarget in $solutionTargets) {
    & $solutionScriptPath -SolutionTarget $solutionTarget -Configuration Release -Platform x64
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}
