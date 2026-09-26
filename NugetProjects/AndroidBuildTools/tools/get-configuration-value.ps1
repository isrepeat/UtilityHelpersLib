[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$ProjectRoot,

    [Parameter(Mandatory)]
    [string]$Name
)

$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'ProjectConfiguration.ps1')
$config = Read-AndroidBuildConfiguration $ProjectRoot
Get-AndroidBuildConfigurationValue -Configuration $config -Name $Name