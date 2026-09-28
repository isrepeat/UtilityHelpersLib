# AndroidBuildTools

Версионируемые PowerShell-команды, CMake-модули, Gradle conventions и шаблон
нативного Android-приложения. Требуется Windows, PowerShell 5.1+, NuGet CLI,
Visual Studio C++ с CMake, Android SDK/NDK и JDK 21.

## Новый проект без существующего Android-проекта

Для первого проекта ещё нет `build.ps1`, который мог бы восстановить пакет.
Поэтому сначала NuGet распаковывает AndroidBuildTools во временный каталог:

```powershell
$version = '1.0.40'
if ([string]::IsNullOrWhiteSpace($env:UH_PACKAGES_FEED)) {
    throw 'Set UH_PACKAGES_FEED before creating an Android project.'
}
$bootstrapDirectory = Join-Path $env:UH_PACKAGES_FEED '!TEMP'

chcp 65001 | Out-Null
$utf8 = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = $utf8
$OutputEncoding = $utf8

nuget install AndroidBuildTools `
    -Version $version `
    -Source $env:UH_PACKAGES_FEED `
    -OutputDirectory $bootstrapDirectory `
    -ForceEnglishOutput `
    -NonInteractive

$tools = Join-Path $bootstrapDirectory "AndroidBuildTools.$version"
& "$tools\tools\New-AndroidApplication.ps1" `
    -Name SampleApp `
    -PackageId com.example.sampleapp `
    -Destination C:\Projects\SampleApp `
    -BuildToolsSource $env:UH_PACKAGES_FEED `
    -NativePackageSource $env:UH_PACKAGES_FEED `
    -SecretsRoot C:\WORK\Secrets\Android `
    -DriveOAuthClientPath C:\WORK\Secrets\apkupdater-drive-oauth.json `
    -DriveTokenPath C:\WORK\Secrets\apkupdater-drive-token.json `
    -GoogleCloudProject androidappsstorage

cd C:\Projects\SampleApp
./build.ps1 build-android -Configuration Debug
```

Генератор принимает только новый каталог и создаёт работающее приложение с Java
Activity и JNI-библиотекой. Gradle wrapper 9.5.0 включён вместе с SHA-256
дистрибутива. Отдельная установка Gradle не нужна. Имя проекта — латинские буквы
и цифры, начиная с заглавной буквы; package ID — строчные буквы и цифры с точками.

`-SecretsRoot` — обязательный параметр. Генератор создаёт в нём каталог с package ID:
`C:\WORK\Secrets\Android\com.example.sampleapp`. В нём лежат отдельные Debug и
Release keystore, а также `signing.properties` с паролями. Этот каталог находится
вне проекта и не попадает в Git. Обе конфигурации Gradle используют эти ключи.

Генератор получает SHA-1 обоих сертификатов, записывает их в
`Google-OAuth-setup.md` и открывает страницу Google Cloud Clients, если задан
`-GoogleCloudProject`. В Google Console остаётся создать два Android OAuth client:
`SampleApp debug` и `SampleApp release`, с тем же package ID и соответствующим
SHA-1. Desktop OAuth client и refresh token для публикации APK остаются общими для
всех приложений; у приложения меняется только `Drive.Path`.

Генератор сразу создаёт секцию `Drive` в `android-build.psd1`. Пути к Desktop
OAuth JSON и refresh token всегда передаются явно через `-DriveOAuthClientPath` и
`-DriveTokenPath`: генератор не предполагает структуру секретов ApkUpdater.

Полный контракт проекта, разбор bootstrap и flow сборки описаны в
[BUILD-PIPELINE.md](BUILD-PIPELINE.md).

## Синтаксис шаблонов

Во всех шаблонах используется единый синтаксис `{{Имя}}`. Он одинаков для
содержимого файлов и для имён файлов и каталогов, потому что фигурные скобки
допустимы в путях Windows. Генератор заменяет `{{Application}}`,
`{{application}}`, `{{APPLICATION}}`, `{{PackageId}}`, `{{PackagePath}}`,
`{{JniPackage}}`, `{{BuildToolsSource}}`, `{{NativePackageSource}}` и
`{{SigningProperties}}` значениями
параметров создания приложения.

Например, `{{Application}}.Android/src/main/java/{{PackagePath}}` при имени
`SampleApp` и package ID `com.example.sampleapp` становится
`SampleApp.Android/src/main/java/com/example/sampleapp`.
## Границы ответственности

- Приложение: package ID, исходники, ресурсы, пути, native targets, свои зависимости.
- `android-build.psd1`: закреплённая версия пакета, источник NuGet, имена модулей,
  пути и необязательные параметры XAML, previewer, signing и Drive.
- Пакет: реализация сборки, поиск инструментов, вычисление версии APK, Gradle defaults.
- Шаблон: минимальные загрузчики и начальные файлы приложения.

Общие PowerShell-функции находятся в модуле `Module.AndroidBuildTools`.
Scripts подключают его через `Import-Module`; при необходимости команду можно
вызвать явно как `Module.AndroidBuildTools\Resolve-XamlCompiler`.

`Initialize-AndroidBuildConsole` задаёт UTF-8 без BOM для консоли PowerShell,
`$OutputEncoding` и pipeline во внешние программы. Все точки входа пакета
вызывают её после загрузки модуля. Кодировку, с которой конкретный внешний `.exe`
сам формирует свой вывод, эта настройка изменить не может.

`build.ps1` восстанавливает пакет в каталог
`PackageDirectories.AndroidBuildTools` из `android-build.psd1` и получает
параметры команд из пакета. `ANDROID_BUILD_TOOLS_SOURCE` переопределяет источник
NuGet. Распакованные файлы пакета не редактируют. CMake toolchain и Gradle settings
вызывают тот же загрузчик, поэтому работают и при прямом запуске из IDE.

## Gradle conventions

`com.isrepeat.android.application` и `com.isrepeat.android.settings` поставляются
в каталоге `gradle` и подключаются через `pluginManagement.includeBuild`.
Версия AndroidBuildTools одновременно закрепляет PowerShell, CMake и conventions.
AGP 9.3.2, compile/target SDK 36, min SDK 24, ARM64, Java 11 и AndroidX
activity/lifecycle настроены в application plugin. Kotlin встроен в AGP 9 и
наследует JVM target Java. Приложение может переопределять Android DSL после plugin.

Settings plugin задаёт Google, Maven Central и общий Maven feed.
`-PandroidMavenSource` или `ANDROID_MAVEN_SOURCE` меняют локальный default
`C:/!PackagesFeed/Android`. Для динамических версий отключён длительный Gradle cache.

Пути native `.so` задаются в `android.sourceSets` приложения либо через
`-PappNativeLibraries` относительно корня репозитория. Build output модуля:
`Build/<Module>`. Стандартная структура launcher: `Tools/Gradle`.

## Signing

Генератор создаёт Debug и Release keystore за пределами Git и добавляет абсолютный
путь к их `signing.properties` в `SigningProperties` файла `android-build.psd1`.
Gradle получает его как `-PandroidSigningProperties`; значение
`ANDROID_SIGNING_PROPERTIES` остаётся способом переопределить путь в CI. Файл
содержит поля Release (`storeFile`, `storePassword`, `keyAlias`, `keyPassword`) и
Debug (`debugStoreFile`, `debugStorePassword`, `debugKeyAlias`,
`debugKeyPassword`).

## Update из Google Drive

Шаблон нового приложения добавляет Kotlin `MainPage` с кнопкой **Update** и
`GoogleDriveUpdateController` из `androidappkit`. Controller ищет APK в
`Android/<Application>` по имени `<Application>-<major>.<minor>.<patch>.apk`.
Android OAuth client Debug и Release создаются генератором в документации
`Google-OAuth-setup.md`.

Установка выполняется внешним `ApkUpdater`, поэтому его APK и обновляемое
приложение должны быть подписаны одним сертификатом для соответствующей
конфигурации. В опубликованный список целей ApkUpdater добавляют package ID нового
приложения. Доступ к Drive для сборочного uploader-а настраивается отдельно через
секцию `Drive`: Desktop OAuth client и refresh token не входят в Android OAuth.

## Команды

- `restore`: путь восстановленного пакета.
- `build-android`: Debug/Release, `-Clean`, `-NativeOnly`, версия через пару
  `-AppVersionCode` / `-AppVersionName`.
- `build-and-distribute -Destination Local|Drive`: версионные APK.
- `build-for-drive`, `build-all`: сборка и публикация в Drive.
- `bump-version`: вычислить следующую версию без записи в исходники.
- `tools\Remove-AndroidProject.ps1`: запросить package ID и удалить указанный
  проект вместе с его secrets.
- `generate-xaml`: необязательная секция `Xaml`; без неё генерация пропускается.
- `run-android-app-previewer -BuildOnly`: собрать plugin и desktop host без запуска.
- `upload-apk-to-drive`: загрузить APK в `Drive.Path`.

Для XAML задаются `Application`, `UI`, `Xaml.Namespace`, `ControlNamespace`,
`ControlIncludePrefix`. Генератор восстанавливает `XamlRuntime` в
`PackageDirectories.XamlRuntime` и запускает включённый в него
`tools/win-x64/XamlCompiler.exe`. Preview содержит `ArtifactDirectory`, `Root`,
`ProjectFile`, `Executable`, `Plugin`, `Target`. Приложения без этих функций не
обязаны содержать фиктивные пути.

## Версии APK

`VERSION_NAME_BASE=major.minor`, `VERSION_CODE_BASE=major*1000000+minor*1000`.
Patch определяется по файлам в `DistributionDirectory`; `-KeepVersion` сохраняет
последний patch. Обычная сборка использует patch 0. Следующая distribution-сборка
без существующих APK использует patch 1. Исходный version.properties не изменяется.

Имя APK: `<ArtifactName>-<VersionName>.apk`.
Для одной папки distribution сборки выполняются последовательно.

## CMake и публикация

`tools/cmake` содержит Android toolchain, выбор NuGet feed и установку XamlRuntime
и AndroidAppPreviewer.PluginSDK. Native-пакеты распаковываются в
каталоги из `PackageDirectories`; источник задаёт `PackageSources.Native`.

Для публикации измените версию в nuspec и шаблоне конфигурации, задайте
`UH_PACKAGES_FEED` или укажите `Pack.ps1 -FeedPath <путь>`, затем обновите
BuildToolsVersion потребителя.
Опубликованную версию заменять запрещено. При разработке используйте отдельный
временный feed и проверяйте восстановление в новом проекте.