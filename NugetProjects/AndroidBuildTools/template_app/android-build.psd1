@{
    BuildToolsVersion = '1.0.40'
    BuildToolsSource = '{{BuildToolsSource}}'
    ArtifactName = '{{Application}}'
    AndroidModule = '{{Application}}.Android'
    AndroidHost = '{{Application}}.AndroidHost'
    Application = '{{Application}}.Application'
    UI = '{{Application}}.UI'
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
    Xaml = @{
        Namespace = 'urn:{{application}}:xaml'
        ControlNamespace = '{{application}}::ui::control'
        ControlIncludePrefix = '{{Application}}.UI/Control'
    }
    Preview = @{
        ArtifactDirectory = 'Build\{{Application}}.PreviewPlugin'
        Root = '..\AndroidAppPreviewer'
        ProjectFile = 'AndroidAppPreviewer.WPF\AndroidAppPreviewer.WPF.csproj'
        Executable = '!VS_TMP\Build\{Configuration}\x64\AndroidAppPreviewer.WPF\AndroidAppPreviewer.exe'
        Plugin = 'Build\{Configuration}\x64\{{Application}}.PreviewPlugin\{{Application}}.PreviewPlugin.dll'
        Target = '{{application}}_preview_plugin'
    }
    SigningProperties = '{{SigningProperties}}'
    Drive = @{
        Path = @('Android', '{{Application}}')
        OAuthClientPath = '{{DriveOAuthClientPath}}'
        TokenPath = '{{DriveTokenPath}}'
    }
}