# Добавить путь к модулям во временный PSModulePath
$modulePath = Join-Path $PSScriptRoot "Modules"
if (-not ($env:PSModulePath -split ';' | Where-Object { $_ -eq $modulePath })) {
    $env:PSModulePath = "$modulePath;$env:PSModulePath"
}

Import-Module -Name MessagingModule -Prefix m::


$SourceSolutionPath = '"..\UtilityHelpersLib.sln"'

$TargetSolutionPath = Read-Host "Enter path to the target .sln file"
$TargetSolutionPath = m::WrapInQuotesIfNeeded $TargetSolutionPath
if ([string]::IsNullOrWhiteSpace($TargetSolutionPath) -or $TargetSolutionPath -eq '""') {
    m::Message -color Red -Text "no path provided"
	exit 1
}

$FoldersToInsertArgs = @(
    '-f', '3rdParty'
)

$ProjectsToInsertArgs = @(
    '-p', 'ComAPI',
    '-p', 'ComAPI.Shared',
    '-p', 'Helpers.Raw',
    '-p', 'Helpers.Shared',
    '-p', 'Helpers.Includes'
)


$SolutionInjectorExtraArgs = @()
#$RootNameOpt = Read-Host "Enter root name (root solution folder for injected items)"
$RootNameOpt = "UtilityHelpersLib [submodule]"
$RootNameOpt = m::WrapInQuotesIfNeeded $RootNameOpt
if ([string]::IsNullOrWhiteSpace($RootNameOpt) -or $RootNameOpt -eq '""') {
}
else {
	$SolutionInjectorExtraArgs += @('--rootName', $RootNameOpt)
}

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