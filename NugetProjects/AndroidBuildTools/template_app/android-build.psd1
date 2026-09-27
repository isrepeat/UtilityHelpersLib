@{
    BuildToolsVersion = '1.0.39'
    BuildToolsSource = '{{BuildToolsSource}}'
    ArtifactName = '{{Application}}'
    AndroidModule = '{{Application}}.Android'
    AndroidHost = '{{Application}}.AndroidHost'
    NativeLibrary = 'lib{{application}}.so'
    AndroidPresetPrefix = 'android-arm64'
    CMakeVersionVariable = '{{APPLICATION}}_PACKAGE_VERSION'
    GradleRoot = 'Tools\Gradle'
    VersionFile = 'version.properties'
    DistributionDirectory = 'Build\distribution'
    PackageDirectories = @{
        AndroidBuildTools = 'Build\Packages\{{Application}}'
        XamlRuntime = 'Build\Packages\{{Application}}.AndroidHost'
        AndroidAppPreviewerPluginSdk = 'Build\Packages\{{Application}}.PreviewPlugin'
    }
    PackageSources = @{
        Native = '{{NativePackageSource}}'
    }
    SigningProperties = '{{SigningProperties}}'
    Drive = @{
        Path = @('Android', '{{Application}}')
        OAuthClientPath = '{{DriveOAuthClientPath}}'
        TokenPath = '{{DriveTokenPath}}'
    }
}