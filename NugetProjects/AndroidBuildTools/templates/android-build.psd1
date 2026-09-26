@{
    # Закреплённая версия общих инструментов.
    BuildToolsVersion = '1.0.5'
    BuildToolsSource = 'C:\NugetFeed'

    ArtifactName = '<Application>'
    AndroidModule = '<Application>.Android'
    AndroidHost = '<Application>.AndroidHost'
    Application = '<Application>.Application'
    UI = '<Application>.UI'
    NativeLibrary = 'lib<Application>.so'
    AndroidPresetPrefix = 'android-arm64'
    CMakeVersionVariable = '<APPLICATION>_PACKAGE_VERSION'
    GradleRoot = 'Tools\Gradle'
    VersionFile = 'version.properties'
    DistributionDirectory = 'Build\distribution'

    Xaml = @{
        CompilerSource = '<Path to XamlCompiler>'
        CompilerBuild = 'Build\<Application>.Application\xaml-compiler'
        Namespace = 'urn:<application>:xaml'
        ControlNamespace = '<application>::ui::control'
        ControlIncludePrefix = '<Application>.UI/Control'
    }

    Preview = @{
        ArtifactDirectory = 'Build\<Application>.PreviewPlugin'
        Root = '..\AndroidAppPreviewer'
        ProjectFile = 'AndroidAppPreviewer.WPF\AndroidAppPreviewer.WPF.csproj'
        Executable = '!VS_TMP\Build\{Configuration}\x64\AndroidAppPreviewer.WPF\AndroidAppPreviewer.exe'
        Plugin = 'Build\{Configuration}\x64\<Application>.PreviewPlugin\<Application>.PreviewPlugin.dll'
        Target = '<application>_preview_plugin'
    }

    Drive = @{
        Path = @('Android', '<Application>')
        OAuthClientPath = 'C:\WORK\Secrets\apkupdater-drive-oauth.json'
        TokenPath = 'C:\WORK\Secrets\apkupdater-drive-token.json'
    }
}