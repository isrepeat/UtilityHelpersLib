[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$SolutionTarget,

    [ValidateSet('Debug', 'Release')]
    [string]$Configuration,

    [ValidateSet('Win32', 'x64', 'x86')]
    [string]$Platform
)

$ErrorActionPreference = 'Stop'

$modulePath = Join-Path $PSScriptRoot 'Modules\MessagingModule\MessagingModule.psm1'
Import-Module -Name $modulePath -Prefix m:: -ErrorAction Stop

$solutionPath = (Resolve-Path (Join-Path $PSScriptRoot '..\..\UtilityHelpersLib.Nugets.sln')).Path
$vsWherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'

if (-not (Test-Path -LiteralPath $vsWherePath)) {
    m::MessageError "vswhere.exe not found: $vsWherePath"
    exit 1
}

$msBuildPath = & $vsWherePath -latest -prerelease -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if ([string]::IsNullOrWhiteSpace($msBuildPath) -or -not (Test-Path -LiteralPath $msBuildPath)) {
    m::MessageError 'MSBuild.exe was not found in the installed Visual Studio instances.'
    exit 1
}

if ($Platform -eq 'x86') {
    $Platform = 'Win32'
}

$configurations = if ($Configuration) { @($Configuration) } else { @('Debug', 'Release') }
$platforms = if ($Platform) { @($Platform) } else { @('Win32', 'x64') }
$targetName = $SolutionTarget -replace '\.', '_'

m::Message -color Blue -text "Solution: $solutionPath"
m::Message -color Blue -text "Target: $SolutionTarget"

foreach ($currentConfiguration in $configurations) {
    foreach ($currentPlatform in $platforms) {
        m::MessageAction "Build [$currentConfiguration | $currentPlatform]..."

        & $msBuildPath $solutionPath "/t:$targetName" "/p:Configuration=$currentConfiguration" "/p:Platform=$currentPlatform" '-verbosity:minimal'
        if ($LASTEXITCODE -ne 0) {
            m::MessageError "Build failed [$currentConfiguration | $currentPlatform] with exit code $LASTEXITCODE."
            exit $LASTEXITCODE
        }
    }
}

m::Message -color Green -text 'Build completed.'
