# AndroidBuildTools: flow сборки и контракт проекта

AndroidBuildTools содержит общую инфраструктуру native Android-приложений:
PowerShell-команды, CMake-модули, Gradle convention plugins и шаблон минимального
проекта. Приложение хранит только свои имена, пути, исходники, ресурсы и зависимости.

## 1. Начало без существующего проекта

Если Android-проекта ещё нет, его `build.ps1` использовать нельзя: файла пока не
существует. Сначала распакуйте точную версию NuGet-пакета во временный каталог:

```powershell
$version = '1.0.40'
$bootstrapDirectory = 'C:\Temp\AndroidBuildTools'

chcp 65001 | Out-Null
$utf8 = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = $utf8
$OutputEncoding = $utf8

nuget install AndroidBuildTools `
    -Version $version `
    -Source C:\NugetFeed `
    -OutputDirectory $bootstrapDirectory `
    -NonInteractive
```

NuGet создаст каталог:

```text
C:\Temp\AndroidBuildTools\AndroidBuildTools.1.0.40
```

Запустите generator из этого каталога:

```powershell
$tools = Join-Path $bootstrapDirectory "AndroidBuildTools.$version"
& "$tools\tools\New-AndroidApplication.ps1" `
    -Name SampleApp `
    -PackageId com.example.sampleapp `
    -Destination C:\Projects\SampleApp `
    -BuildToolsSource C:\NugetFeed `
    -NativePackageSource C:\NugetFeed `
    -SecretsRoot C:\WORK\Secrets\Android `
    -DriveOAuthClientPath C:\WORK\Secrets\apkupdater-drive-oauth.json `
    -DriveTokenPath C:\WORK\Secrets\apkupdater-drive-token.json `
    -GoogleCloudProject androidappsstorage
```

После этого существует самостоятельный проект. Его обычная первая проверка:

```powershell
cd C:\Projects\SampleApp
./build.ps1 build-android -Configuration Debug
```

`-SecretsRoot` передаётся явно. Генератор создаёт
`<SecretsRoot>\<package ID>\debug.keystore`, `release.keystore` и
`signing.properties`, генерирует оба ключа через `keytool` и записывает путь к
properties в проектный `android-build.psd1`. В Git не попадают ни ключи, ни пароли.

`Google-OAuth-setup.md` содержит package ID и SHA-1 обоих сертификатов.
`-GoogleCloudProject` открывает страницу Clients указанного Cloud-проекта. Вручную
создайте два Android OAuth client: Debug и Release. Они нужны приложению для Google
API. Desktop OAuth client и refresh token сборочного uploader-а остаются общими;
для каждого приложения отличается только `Drive.Path`.

Секция `Drive` создаётся вместе с проектом. Путь содержит `Android/<Application>`;
пути к общим Desktop OAuth JSON и refresh token передаются явно параметрами
`-DriveOAuthClientPath` и `-DriveTokenPath`.

## 2. Минимальный контракт проекта

В корне приложения нужны `build.ps1` и `android-build.psd1`. Первый файл —
bootstrap: он читает второе, восстанавливает закреплённую версию пакета и передаёт
ей команду. Его не нужно дополнять логикой сборки.

`android-build.psd1` задаёт данные конкретного приложения. Минимальный шаблон
создаёт такие обязательные значения:

```powershell
@{
    BuildToolsVersion = '1.0.40'
    BuildToolsSource = 'C:\NugetFeed'
    ArtifactName = 'SampleApp'
    AndroidModule = 'SampleApp.Android'
    AndroidHost = 'SampleApp.AndroidHost'
    NativeLibrary = 'libsampleapp.so'
    AndroidPresetPrefix = 'android-arm64'
    CMakeVersionVariable = 'SAMPLEAPP_PACKAGE_VERSION'
    GradleRoot = 'Tools\Gradle'
    VersionFile = 'version.properties'
    DistributionDirectory = 'Build\distribution'
    PackageDirectories = @{
        AndroidBuildTools = 'Build\Packages\SampleApp'
        XamlRuntime = 'Build\Packages\SampleApp.AndroidHost'
        AndroidAppPreviewerPluginSdk = 'Build\Packages\SampleApp.PreviewPlugin'
    }
    PackageSources = @{
        Native = 'C:\NugetFeed'
    }
}
```

Пути могут быть любыми относительными путями от корня проекта. AndroidBuildTools
преобразует их в абсолютные значения и использует только переданные параметры.
Пакет не хранит имена модулей или относительные пути отдельного приложения.

`PackageDirectories.AndroidBuildTools` нужен всегда. `XamlRuntime` требуется при
добавлении секции `Xaml`; `AndroidAppPreviewerPluginSdk` — при подключении
previewer-а. `PackageSources.Native` задаёт source для XamlRuntime и SDK previewer-а.

### Необязательные возможности

Для XAML добавляются имена исходных каталогов и правила генерации:

```powershell
Application = 'SampleApp.Application'
UI = 'SampleApp.UI'
Xaml = @{
    Namespace = 'urn:sampleapp:xaml'
    ControlNamespace = 'sampleapp::ui::control'
    ControlIncludePrefix = 'SampleApp.UI/Control'
}
```

Для previewer-а и Drive добавляются секции `Preview` и `Drive`. Генератор добавляет
`SigningProperties` автоматически: это абсолютный путь к файлу секретов вне Git.

## 3. Обычная Android-сборка

```powershell
./build.ps1 build-android -Configuration Debug
```

Flow выглядит так:

```text
build.ps1
  ↓ читает android-build.psd1
NuGet restore AndroidBuildTools.<версия>
  ↓
tools/Invoke-Build.ps1
  ↓
build-android.ps1
  ├─ generate-xaml.ps1, если есть Xaml
  ├─ CMake + NDK собирают native .so
  └─ Gradle wrapper упаковывает APK
```

`build.ps1` восстанавливает пакет в `PackageDirectories.AndroidBuildTools`.
Команда `restore` возвращает путь к распакованному пакету; CMake и Gradle используют
её, чтобы найти ту же закреплённую версию инфраструктуры.

## 4. XAML

`build-android.ps1` запускает `generate-xaml.ps1` до CMake. Если в конфигурации
нет секции `Xaml`, script завершается без действий.

При наличии XAML generator:

1. восстанавливает `XamlRuntime` в `PackageDirectories.XamlRuntime`;
2. выбирает самую новую папку `XamlRuntime.<версия>`;
3. запускает `tools\win-x64\XamlCompiler.exe` из этого пакета;
4. создаёт `.xaml.cpp` и `.xaml.h` в `!Generated` только для изменённых XAML.

Исходники XamlCompiler не требуются проекту и не компилируются при сборке приложения.

## 5. CMake

CMake presets приложения подключают bootstrap `cmake/AndroidToolchain.cmake`.
Он выполняет `build.ps1 restore` и подключает CMake-модули из AndroidBuildTools.
Общие модули получают каталоги package restore и source из `android-build.psd1` через
команду `build.ps1 get-configuration-path` или `get-configuration-value`.

Так CMake, PowerShell и XAML-generator используют одни и те же значения из
конфигурации проекта. Для Android CMake собирает native `.so`; Gradle только берёт
готовую библиотеку из настроенного `jniLibs`-каталога и кладёт её в APK.

## 6. Gradle convention plugins

`Tools/Gradle/settings.gradle.kts` запускается до Android-модулей. Он вызывает
`build.ps1 restore`, получает путь AndroidBuildTools и подключает его Gradle-каталог:

```kotlin
includeBuild("$packageRoot/gradle")
```

Это included build: Gradle рассматривает каталог `gradle` внутри NuGet-пакета как
локальный Gradle-проект, который предоставляет plugins. Их не нужно публиковать в
Gradle Plugin Portal.

Доступны:

```kotlin
id("com.isrepeat.android.settings")
id("com.isrepeat.android.application")
```

Settings plugin задаёт repositories. Application plugin задаёт SDK-версии,
ARM64, Java/Kotlin options, AndroidX-зависимости, Debug/Release, versionCode,
versionName, signing policy и каталог build output. Файл Android-модуля оставляет
у себя `applicationId`, `namespace`, ресурсы, native `.so` и специфичные зависимости.

## 7. Общий PowerShell-модуль

Команды пакета импортируют `Module.AndroidBuildTools`:

```powershell
Import-Module -Name (Join-Path $PSScriptRoot `
    'Modules\Module.AndroidBuildTools\Module.AndroidBuildTools.psm1') `
    -ErrorAction Stop
```

Экспортированные функции вызываются с явной квалификацией:

```powershell
$config = Module.AndroidBuildTools\Read-AndroidBuildConfiguration $ProjectRoot
```

Это показывает, что функция принадлежит общему пакету. PowerShell не имеет
настоящих namespace, но запись `Module.AndroidBuildTools\ИмяФункции` даёт сходный
эффект и исключает неоднозначность источника команды.

## 8. Команды

| Задача | Команда |
| --- | --- |
| Путь пакета | `./build.ps1 restore` |
| Debug APK | `./build.ps1 build-android -Configuration Debug` |
| Release APK | `./build.ps1 build-android -Configuration Release` |
| Только native `.so` | `./build.ps1 build-android -NativeOnly` |
| Новый CMake configure | `./build.ps1 build-android -Clean` |
| Генерация XAML | `./build.ps1 generate-xaml` |
| Versioned APK | `./build.ps1 build-and-distribute -Destination Local` |
| Загрузка в Drive | `./build.ps1 build-and-distribute -Destination Drive` |
| Previewer без окна | `./build.ps1 run-android-app-previewer -BuildOnly` |

## 9. Версии APK и подпись

`version.properties` содержит базовую версию. `build-and-distribute` выбирает
следующий patch по APK в `DistributionDirectory`, передаёт versionCode/versionName
в CMake и Gradle и не меняет tracked-файлы.

Генератор создаёт отдельные Debug и Release keystore в `SecretsRoot` и передаёт
сгенерированный `signing.properties` через `SigningProperties`. Gradle передаёт
путь как `-PandroidSigningProperties`. Для CI его можно переопределить переменной
`ANDROID_SIGNING_PROPERTIES`.

## 10. Update из Google Drive

Новый шаблон содержит Kotlin `MainPage` с кнопкой **Update**. Она запускает
`GoogleDriveUpdateController` из `androidappkit`: controller авторизует пользователя
в Google Drive, ищет versioned APK в `Android/<Application>` и проверяет package ID
и версию до передачи файла updater-у.

Updater — отдельное приложение `ApkUpdater`. Для вызова оно и целевое приложение
подписываются одним сертификатом; Android проверяет это через signature permission.
Кроме того, опубликованный список целей ApkUpdater должен включать package ID
приложения. Без этих двух условий кнопка может проверить Drive, но установка будет
остановлена безопасно.

## 11. Диагностика

1. Не восстановился AndroidBuildTools — проверить `BuildToolsVersion`,
   `BuildToolsSource`, `PackageDirectories.AndroidBuildTools` и `nuget.exe`.
2. Не найден XamlCompiler — проверить `PackageSources.Native` и
   `PackageDirectories.XamlRuntime`.
3. Не собрался native target — запустить `build-android -NativeOnly` и проверить
   CMake, NDK, CMake preset и название `.so`.
4. Native `.so` есть, APK нет — проверить Gradle, Android module и `jniLibs`.
5. Не собирается Release — проверить signing properties и keystore.