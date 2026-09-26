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
Resolve-AndroidBuildConfigurationPath -Configuration $config -ProjectRoot $ProjectRoot -Name $Name