# AndroidTemplates

Версионируемые шаблоны и PowerShell-команды жизненного цикла native Android-приложений. Пакет создаёт проект, а `AndroidBuildTools` выполняет его сборку.

## Создание проекта

Сначала установите `AndroidTemplates` во временный каталог. Генератор найдёт в `BuildToolsSource` последнюю опубликованную версию `AndroidBuildTools` и запишет её в `android-build.psd1` создаваемого проекта.

```powershell
$bootstrapDirectory = Join-Path $env:UH_PACKAGES_FEED '!TEMP'

nuget install AndroidTemplates `
    -Source $env:UH_PACKAGES_FEED `
    -OutputDirectory $bootstrapDirectory `
    -ForceEnglishOutput `
    -NonInteractive

$tools = Get-ChildItem -LiteralPath $bootstrapDirectory -Directory |
    Where-Object { $_.Name -match '^AndroidTemplates\.(.+)$' } |
    Sort-Object { [version]$_.Name.Substring('AndroidTemplates.'.Length) } -Descending |
    Select-Object -First 1 -ExpandProperty FullName
& "$tools\tools\New-AndroidApplication.ps1" `
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

Генератор копирует `Remove-AndroidProject.ps1` в `Scripts` созданного проекта. Скрипт удаляет явно указанный корень проекта и каталог секретов, найденный в его `android-build.psd1`. До удаления он запрашивает подтверждение.