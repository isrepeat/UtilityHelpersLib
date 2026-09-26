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
    foreach ($name in @('ArtifactName', 'AndroidModule', 'AndroidHost', 'NativeLibrary', 'AndroidPresetPrefix', 'CMakeVersionVariable', 'GradleRoot', 'VersionFile', 'DistributionDirectory')) {
        if ([string]::IsNullOrWhiteSpace($config[$name])) {
            throw "android-build.psd1 must define $name."
        }
    }
    return $config
}