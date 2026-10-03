[CmdletBinding()]
param()

dynamicparam {
    $ErrorActionPreference = 'Stop'
    # До restore модуль AndroidBuildTools ещё не существует в проекте.
    $utf8Encoding = [System.Text.UTF8Encoding]::new($false)
    [Console]::InputEncoding = $utf8Encoding
    [Console]::OutputEncoding = $utf8Encoding
    $OutputEncoding = $utf8Encoding
    
    $androidProjectConfig = & ([scriptblock]::Create([System.IO.File]::ReadAllText((Join-Path $PSScriptRoot 'android-build.psd1'))))
    . (Join-Path $PSScriptRoot 'Scripts\PowerShell\Read-AndroidSharedProps.ps1')
    $androidProjectSharedConfig = Read-AndroidSharedProps -ProjectRoot $PSScriptRoot
    $packagesRoot = Join-Path $PSScriptRoot $androidProjectConfig.PackageDirectories.AndroidBuildTools
    $packageName = 'AndroidBuildTools'
    $packageVersion = $androidProjectConfig.AndroidBuildToolsVersion
    if ([string]::IsNullOrWhiteSpace($packageVersion)) {
        throw 'android-build.psd1 must define AndroidBuildToolsVersion.'
    }
    $nuget = (Get-Command nuget.exe -ErrorAction Stop).Source
    $source = if ($env:ANDROID_BUILD_TOOLS_SOURCE) { $env:ANDROID_BUILD_TOOLS_SOURCE } else { $androidProjectSharedConfig.Paths.PackagesFeed }

    & $nuget install $packageName -Version $packageVersion -Source $source -OutputDirectory $packagesRoot -NonInteractive -DirectDownload -NoHttpCache -ForceEnglishOutput | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "$packageName $packageVersion restore failed with exit code $LASTEXITCODE."
    }

    $entryPoint = Join-Path $packagesRoot "$packageName.$packageVersion\tools\Invoke-Build.ps1"
    if (-not (Test-Path -LiteralPath $entryPoint -PathType Leaf)) {
        throw "$packageName $packageVersion did not provide tools\Invoke-Build.ps1 in $packagesRoot."
    }

    # Получаем параметры из пакета: загрузчик не содержит списка команд и опций.
    $metadata = Get-Command -Name $entryPoint -ErrorAction Stop
    $common = [System.Management.Automation.PSCmdlet]::CommonParameters + [System.Management.Automation.PSCmdlet]::OptionalCommonParameters
    $parameters = [System.Management.Automation.RuntimeDefinedParameterDictionary]::new()
    foreach ($parameter in $metadata.Parameters.Values) {
        if ($parameter.Name -ne 'ProjectRoot' -and $parameter.Name -notin $common) {
            $parameters.Add($parameter.Name, [System.Management.Automation.RuntimeDefinedParameter]::new($parameter.Name, $parameter.ParameterType, $parameter.Attributes))
        }
    }
    $parameters
}

end {
    # Сценарий приложения проверяет Updater; общий сборщик не знает о зависимых проектах.
    $command = if ($PSBoundParameters.Command) { $PSBoundParameters.Command } else { 'build-android' }
    if ($command -in @('build-android', 'build-and-distribute', 'build-for-drive', 'build-all') -and -not $PSBoundParameters.NativeOnly) {
        $configuration = if ($PSBoundParameters.Configuration) { $PSBoundParameters.Configuration } elseif ($command -eq 'build-android') { 'Debug' } else { 'Release' }
        $signingProperties = if ($PSBoundParameters.SigningProperties) { $PSBoundParameters.SigningProperties } else { $androidProjectSharedConfig.Paths.SigningProperties }
        & (Join-Path $PSScriptRoot 'Scripts\PowerShell\ensure-apk-updater.ps1') -ApkUpdaterProjectRoot $androidProjectSharedConfig.Paths.ApkUpdaterProjectRoot -SigningProperties $signingProperties -BuildToolsRoot (Split-Path -Parent (Split-Path -Parent $entryPoint)) -Configuration $configuration
    }
    & $entryPoint -ProjectRoot $PSScriptRoot @PSBoundParameters
}