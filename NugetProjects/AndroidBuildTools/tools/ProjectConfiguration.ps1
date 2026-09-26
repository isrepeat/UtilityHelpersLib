function Import-AndroidBuildDataFile {
    param([Parameter(Mandatory)] [string]$Path)

    # android-build.psd1 — контролируемый файл конфигурации проекта. Не зависим
    # от Import-PowerShellDataFile: в урезанном окружении CMake этот cmdlet
    # может отсутствовать даже в Windows PowerShell.
    $config = & ([scriptblock]::Create([System.IO.File]::ReadAllText($Path)))
    if ($config -isnot [hashtable]) {
        throw "Configuration must return a hashtable: $Path"
    }
    return $config
}

function Read-AndroidBuildConfiguration {
    param([Parameter(Mandatory)] [string]$ProjectRoot)

    $config = Import-AndroidBuildDataFile (Join-Path $ProjectRoot 'android-build.psd1')
    foreach ($name in @('ArtifactName', 'AndroidModule', 'AndroidHost', 'NativeLibrary', 'AndroidPresetPrefix', 'CMakeVersionVariable', 'GradleRoot', 'VersionFile', 'DistributionDirectory', 'PackageDirectories', 'PackageSources')) {
        if ([string]::IsNullOrWhiteSpace($config[$name])) {
            throw "android-build.psd1 must define $name."
        }
    }
    return $config
}

function Resolve-AndroidBuildConfigurationPath {
    param(
        [Parameter(Mandatory)] [hashtable]$Configuration,
        [Parameter(Mandatory)] [string]$ProjectRoot,
        [Parameter(Mandatory)] [string]$Name
    )

    $value = Get-AndroidBuildConfigurationValue -Configuration $Configuration -Name $Name
    if ([string]::IsNullOrWhiteSpace($value)) {
        throw "android-build.psd1 must define $Name."
    }
    return [System.IO.Path]::GetFullPath((Join-Path $ProjectRoot $value))
}

function Get-AndroidBuildConfigurationValue {
    param(
        [Parameter(Mandatory)] [hashtable]$Configuration,
        [Parameter(Mandatory)] [string]$Name
    )

    $value = $Configuration
    foreach ($segment in $Name.Split('.')) {
        if ($value -isnot [hashtable] -or -not $value.ContainsKey($segment)) {
            throw "android-build.psd1 must define $Name."
        }
        $value = $value[$segment]
    }
    if ([string]::IsNullOrWhiteSpace($value)) {
        throw "android-build.psd1 must define $Name."
    }
    return $value
}