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
& "$tools\PowerShell\New-AndroidApplication.ps1" `
    -Name SampleApp `
    -PackageId com.example.sampleapp `
    -Destination C:\WORK\Android\Projects\SampleApp
```

## Удаление проекта

Генератор копирует `Remove-AndroidProject.ps1` в `Scripts` созданного проекта. Скрипт удаляет явно указанный корень проекта и каталог секретов, найденный в его `android-build.psd1`. До удаления он запрашивает подтверждение.

До генерации создайте `Android.SharedProps.json` над каталогом будущего проекта. Генератор и сборка используют ближайший файл с `SchemaVersion: 1`, `Paths` и необязательным `Properties`. Пути в `Paths` вычисляются от каталога JSON. Полный контракт описан в `Windows/Me/Documentation/AndroidBuild.md`.

`Paths.ApkUpdaterProjectRoot` задаёт отдельный репозиторий Updater. При создании общих ключей генератор собирает Debug и Release ApkUpdater с тем же `Paths.SigningProperties`. Общие пути читаются из JSON перед каждой сборкой. Сценарий `template_app/Scripts/PowerShell/ensure-apk-updater.ps1` проверяет сертификат APK Updater и пересобирает его при отсутствии или несовпадении подписи. AndroidBuildTools предоставляет только универсальную проверку подписи APK.