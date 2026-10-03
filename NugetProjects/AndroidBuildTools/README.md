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

`tools/test-apk-signing.ps1` сравнивает SHA-256 сертификата переданного APK с ключом выбранной конфигурации и возвращает boolean. Координация зависимых приложений находится в сценариях AndroidTemplates.

Секция `Native = @{}` включает CMake и XAML; нативные поля `AndroidHost`, `NativeLibrary`, `AndroidPresetPrefix` и `CMakeVersionVariable` остаются в конфигурации проекта. Для совместимости наличие старого `NativeLibrary` также включает нативную сборку. Если этих настроек нет, выполняется только Gradle и Visual Studio не требуется. `-NativeOnly` для такого проекта завершается явной ошибкой.

Kotlin-приложения используют те же команды через свой `build.ps1`. Общий uploader принимает `-FilePath` и `-MimeType` и возвращает метаданные Drive с ID файла; старое имя `-ApkPath` поддерживается как alias.