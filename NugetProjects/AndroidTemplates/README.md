# AndroidTemplates

Версионируемые шаблоны и PowerShell-команды жизненного цикла native Android-приложений. Пакет создаёт проект, а `AndroidBuildTools` выполняет его сборку.

## Создание проекта

Сначала установите `AndroidTemplates` во временный каталог. Версию `AndroidBuildTools` передайте явно: она будет записана в `android-build.psd1` создаваемого проекта и больше не зависит от переменных среды Windows.

```powershell
$templatesVersion = '1.0.0.1'
$buildToolsVersion = '1.0.63.1'
$bootstrapDirectory = Join-Path $env:UH_PACKAGES_FEED '!TEMP'

nuget install AndroidTemplates `
    -Version $templatesVersion `
    -Source $env:UH_PACKAGES_FEED `
    -OutputDirectory $bootstrapDirectory `
    -ForceEnglishOutput `
    -NonInteractive

$tools = Join-Path $bootstrapDirectory "AndroidTemplates.$templatesVersion"
& "$tools\tools\New-AndroidApplication.ps1" `
    -BuildToolsVersion $buildToolsVersion `
    -Name SampleApp `
    -PackageId com.example.sampleapp `
    -Destination C:\Projects\SampleApp `
    -BuildToolsSource $env:UH_PACKAGES_FEED `
    -NativePackageSource $env:UH_PACKAGES_FEED `
    -SecretsRoot C:\WORK\Secrets\Android `
    -DriveOAuthClientPath C:\WORK\Secrets\apkupdater-drive-oauth.json `
    -DriveTokenPath C:\WORK\Secrets\apkupdater-drive-token.json
```

## Удаление проекта

Генератор копирует `Remove-AndroidProject.ps1` в `Scripts` созданного проекта. Скрипт удаляет явно указанный корень проекта и каталог секретов, найденный в его `android-build.psd1`. До удаления он запрашивает package ID и подтверждение.