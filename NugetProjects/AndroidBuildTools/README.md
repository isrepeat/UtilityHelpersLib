# AndroidBuildTools

Версионируемые PowerShell-команды, CMake-модули, Gradle conventions и шаблон
нативного Android-приложения. Требуется Windows, PowerShell 5.1+, NuGet CLI,
Visual Studio C++ с CMake, Android SDK/NDK и JDK 21.

## Новый проект

После восстановления пакета выполните:

```powershell
./tools/New-AndroidApplication.ps1 -Name SampleApp -PackageId com.example.sampleapp -Destination C:\Projects\SampleApp -BuildToolsSource C:\NugetFeed
cd C:\Projects\SampleApp
./build.ps1 build-android -Configuration Debug
```

Генератор принимает только новый каталог и создаёт работающее приложение с Java
Activity и JNI-библиотекой. Gradle wrapper 9.5.0 включён вместе с SHA-256
дистрибутива. Отдельная установка Gradle не нужна. Имя проекта — латинские буквы
и цифры, начиная с заглавной буквы; package ID — строчные буквы и цифры с точками.

## Границы ответственности

- Приложение: package ID, исходники, ресурсы, пути, native targets, свои зависимости.
- `android-build.psd1`: закреплённая версия пакета, источник NuGet, имена модулей,
  пути и необязательные параметры XAML, previewer, signing и Drive.
- Пакет: реализация сборки, поиск инструментов, вычисление версии APK, Gradle defaults.
- Шаблон: минимальные загрузчики и начальные файлы приложения.

`build.ps1` восстанавливает пакет в `Build/Packages/<ArtifactName>` и получает
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

Debug использует стандартный debug keystore. Release требует внешнего properties
файла с `storeFile`, `storePassword`, `keyAlias`, `keyPassword` либо явного
`android.buildTypes.release.signingConfig` в приложении.

Путь передаётся через `SigningProperties` в `android-build.psd1`,
`-PandroidSigningProperties` или `ANDROID_SIGNING_PROPERTIES`.
Относительный `storeFile` отсчитывается от Android-модуля. Отсутствие signing
не мешает Debug, а Release останавливается до упаковки. Ключи и пароли не входят
в пакет и шаблон. Смена ключа не является частью миграции сборки.

## Команды

- `restore`: путь восстановленного пакета.
- `build-android`: Debug/Release, `-Clean`, `-NativeOnly`, версия через пару
  `-AppVersionCode` / `-AppVersionName`.
- `build-and-distribute -Destination Local|Drive`: версионные APK.
- `build-for-drive`, `build-all`: сборка и публикация в Drive.
- `bump-version`: вычислить следующую версию без записи в исходники.
- `generate-xaml`: необязательная секция `Xaml`; без неё генерация пропускается.
- `run-android-app-previewer -BuildOnly`: собрать plugin и desktop host без запуска.
- `upload-apk-to-drive`: загрузить APK в `Drive.Path`.

Для XAML задаются `Application`, `UI`, `Xaml.Namespace`, `ControlNamespace`,
`ControlIncludePrefix`. Генератор восстанавливает `XamlRuntime` в
`Build/Packages/<AndroidHost>` и запускает включённый в него
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
`Build/Packages/<Project>`. `ANDROIDAPPKIT_NUGET_SOURCE` задаёт их источник.

Для публикации измените версию в nuspec и шаблоне конфигурации, выполните
`Pack.ps1 -FeedPath C:\NugetFeed`, затем обновите BuildToolsVersion потребителя.
Опубликованную версию заменять запрещено. При разработке используйте отдельный
временный feed и проверяйте восстановление в новом проекте.