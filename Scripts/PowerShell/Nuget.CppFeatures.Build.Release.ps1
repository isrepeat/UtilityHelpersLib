$ErrorActionPreference = 'Stop'

$solutionScriptPath = Join-Path $PSScriptRoot 'Solution.UtilityHelpersLib.Nugets.ps1'
$solutionTarget = 'NugetProjects\CppFeatures\Nuget\CppFeatures.WinRt.Projection'

& $solutionScriptPath -SolutionTarget $solutionTarget -Platform x64
exit $LASTEXITCODE
