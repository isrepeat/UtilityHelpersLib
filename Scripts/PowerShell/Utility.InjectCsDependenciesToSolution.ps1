# Ќазначаем рабочим каталогом текущий каталог скрипта 
# (все относительные пути далее будут относительно этого каталога).
Set-Location -Path $PSScriptRoot

# ƒобавл€ем в PSModulePath путь к кастомным модул€м,
# чтобы подключить их по названию модул€ (вместо абсолютного пути).
$modulePath = Join-Path $PSScriptRoot "Modules"
if (-not ($env:PSModulePath -split ';' | Where-Object { $_ -eq $modulePath })) {
    $env:PSModulePath = "$modulePath;$env:PSModulePath"
}

# --- Import ---
Import-Module -Name MessagingModule -Prefix m:: -ErrorAction Stop

# --- Main ---
$SourceSolutionPath = '"..\..\UtilityHelpersLib.sln"'

$TargetSolutionPath = Read-Host "Enter path to the target .sln file"
$TargetSolutionPath = m::WrapInQuotesIfNeeded $TargetSolutionPath
if ([string]::IsNullOrWhiteSpace($TargetSolutionPath) -or $TargetSolutionPath -eq '""') {
    m::Message -color Red -Text "no path provided"
	exit 1
}

$FoldersToInsertArgs = @(
    '-f', 'CodeAnalyzer',
    '-f', 'HelpersCs'
)

$ProjectsToInsertArgs = @(
)

$SolutionInjectorExtraArgs = @()

#$RootNameOpt = Read-Host "Enter root name (root solution folder for injected items)"
$RootNameOpt = "UtilityHelpersLib [submodule]"
$RootNameOpt = m::WrapInQuotesIfNeeded $RootNameOpt
if (-not [string]::IsNullOrWhiteSpace($RootNameOpt) -and
	$RootNameOpt -ne '""'
) {
	$SolutionInjectorExtraArgs += @('--rootName', $RootNameOpt)
}

$SolutionConfigsToRemoveArgs = @(
    "-rmCfg", "Debug|ARM",
	"-rmCfg", "Debug|ARM64",
	"-rmCfg", "Release|ARM",
	"-rmCfg", "Release|ARM64"
)
$SolutionInjectorExtraArgs += $SolutionConfigsToRemoveArgs

# --- Show args ---
m::Message -color Blue -Text "Source solution: $SourceSolutionPath"
m::Message -color Blue -Text "Target solution: $TargetSolutionPath"
m::Message -color Blue -Text "Folders to insert: $($FoldersToInsertArgs -join ' ')"
m::Message -color Blue -Text "Projects to insert: $($ProjectsToInsertArgs -join ' ')"
m::Message -color Blue -Text "Extra args: $($SolutionInjectorExtraArgs -join ' ')"

# --- Run tool ---
$EXE = "..\Utilities\bin\SolutionInjector.exe"
m::Message -color Yellow -Text "Launching $EXE ..."

& $EXE `
  $SourceSolutionPath `
  $TargetSolutionPath `
  $FoldersToInsertArgs `
  $ProjectsToInsertArgs `
  $SolutionInjectorExtraArgs