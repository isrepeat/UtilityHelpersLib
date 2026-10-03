# AndroidBuildTools

Версионируемые PowerShell-команды, CMake-модули и Gradle conventions для сборки native Android-приложений. Требуется Windows, PowerShell 5.1+, NuGet CLI, Visual Studio C++ с CMake, Android SDK/NDK и JDK 21.

Создание и удаление проектов не входят в этот пакет: для них используется `AndroidTemplates`. При создании генератор находит последнюю опубликованную версию AndroidBuildTools и фиксирует её в `android-build.psd1` нового проекта.

## Границы ответственности

- Приложение: package ID, исходники, ресурсы, пути, native targets и свои зависимости.
- `android-build.psd1`: закреплённая версия пакета, источник NuGet, имена модулей, пути и необязательные параметры XAML, previewer, signing и Drive.
- AndroidBuildTools: реализация сборки, поиск инструментов, вычисление версии APK и Gradle defaults.
- AndroidTemplates: шаблон, создание и удаление проектов.

`build.ps1` восстанавливает пакет в каталог `PackageDirectories.AndroidBuildTools` из `android-build.psd1` и получает параметры команд из пакета. `ANDROID_BUILD_TOOLS_SOURCE` временно переопределяет источник NuGet.

Полный контракт проекта и flow сборки описаны в [BUILD-PIPELINE.md](BUILD-PIPELINE.md).

Общие пути читаются из ближайшего `Android.SharedProps.json` вверх от корня проекта. `Paths` определяет signing, Updater, Drive, feed и previewer; относительные значения вычисляются от каталога JSON. `$androidProjectConfig` содержит только настройки проекта, а `$androidProjectSharedConfig` — отдельные секции `Paths` и `Properties` из JSON. Команды чтения конфигурации работают только с настройками проекта; сборка обращается к общим настройкам напрямую через `$androidProjectSharedConfig`. Полный контракт версии 1 описан в `Windows/Me/Documentation/AndroidBuild.md`.

Перед сборкой Android APK проверяется SHA-256 сертификата ApkUpdater из `Paths.ApkUpdaterProjectRoot` для выбранной конфигурации по `Paths.SigningProperties`. Отсутствующий, повреждённый или подписанный другим ключом APK автоматически пересобирается; результат проверяется повторно. Для `-NativeOnly` проверка не выполняется.