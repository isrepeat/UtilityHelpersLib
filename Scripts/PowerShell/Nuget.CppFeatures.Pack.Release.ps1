$ErrorActionPreference = 'Stop'

$utf8Encoding = [System.Text.UTF8Encoding]::new($false)
[Console]::InputEncoding = $utf8Encoding
[Console]::OutputEncoding = $utf8Encoding
$OutputEncoding = $utf8Encoding

$feedResolver = Join-Path $PSScriptRoot 'Resolve-PackagesFeed.ps1'
$env:UH_PACKAGES_FEED = & $feedResolver
$solutionScriptPath = Join-Path $PSScriptRoot 'Solution.UtilityHelpersLib.Nugets.ps1'
$solutionTarget = 'NugetProjects\CppFeatures\Nuget\CppFeatures.WinRt.Projection'

& $solutionScriptPath -SolutionTarget $solutionTarget -Platform x64
exit $LASTEXITCODE