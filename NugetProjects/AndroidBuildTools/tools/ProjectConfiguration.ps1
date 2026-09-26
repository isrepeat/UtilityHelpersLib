function Read-AndroidBuildConfiguration {
    param([Parameter(Mandatory)] [string]$ProjectRoot)

    $config = Import-PowerShellDataFile (Join-Path $ProjectRoot 'android-build.psd1')
    foreach ($name in @('ArtifactName', 'AndroidModule', 'AndroidHost', 'Application', 'UI', 'NativeLibrary', 'AndroidPresetPrefix', 'CMakeVersionVariable', 'GradleRoot', 'VersionFile', 'DistributionDirectory')) {
        if ([string]::IsNullOrWhiteSpace($config[$name])) {
            throw "android-build.psd1 must define $name."
        }
    }
    return $config
}