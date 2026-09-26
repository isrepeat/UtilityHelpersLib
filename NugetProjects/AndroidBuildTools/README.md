# AndroidBuildTools

Общие инструменты сборки Android-приложений с нативной библиотекой CMake и XAML.
Требуется Windows, PowerShell 5.1+, NuGet CLI, Visual Studio C++, Android SDK/NDK и JDK.
Gradle wrapper, CMake presets и исходники XamlCompiler предоставляет приложение.

## Подключение

В корне приложения хранится `android-build.psd1`: точная версия пакета, источник NuGet,
имена модулей, пути, параметры XAML и Google Drive. Образец конфигурации находится
в DocumentTranslator. Секреты остаются во внешних файлах; конфигурация содержит только пути.

Корневой `build.ps1` восстанавливает закреплённую версию
в игнорируемый каталог `Build/Packages/<ArtifactName>` до запуска CMake. Источник можно переопределить
переменной `ANDROID_BUILD_TOOLS_SOURCE`. Восстановленный пакет не редактируется.

Единый `build.ps1` передаёт `-ProjectRoot` и параметры в соответствующие файлы `tools`.
Он получает список команд, типы и проверки параметров из `tools/Invoke-Build.ps1`
через динамические параметры PowerShell. Новые команды добавляются в пакет,
не требуя изменений загрузчика в приложениях.

При подключении нового проекта скопировать `templates/build.ps1` из исходников
или распакованного пакета в корень приложения и создать `android-build.psd1`.
Шаблон включён в NuGet-пакет, но автоматически не заменяет файлы потребителя.
В приложении загрузчик хранится в Git, чтобы восстановить пакет после клонирования.

Например: `./build.ps1 build-android -Configuration Debug`,
`./build.ps1 build-and-distribute -Destination Local`, `./build.ps1 bump-version`.
Команда `restore` возвращает путь пакета для CMake. Команды `build-for-drive` и
`build-all` вызывают `build-and-distribute -Destination Drive`.

Реализации команд:

- `build-android.ps1`: XAML, CMake, Gradle; поддерживает Debug/Release, Clean, NativeOnly.
- `generate-xaml.ps1`: сборка XamlCompiler и генерация изменённых XAML.
- `bump-version.ps1`: вычисление версии из version.properties и APK в distribution.
- `build-and-distribute.ps1`: сборка версионного APK; `-Destination Local` оставляет его
  на диске, `-Destination Drive` дополнительно загружает на Google Drive.
- `upload-apk-to-drive.ps1`: отдельная загрузка; пути OAuth и назначения обязательны.
- `cmake/AndroidToolchain.cmake`: поиск Android NDK, включая конфигурацию напрямую из IDE.
- `run-android-app-previewer.ps1`: сборка и запуск desktop previewer; `-BuildOnly`
  проверяет сборку без открытия окна. Пути и CMake target задаются в секции Preview.

Версия APK имеет вид major.minor.patch. VERSION_CODE_BASE равен major * 1000000 +
minor * 1000, а versionCode равен этой базе плюс patch. Следующий patch берётся из
имён APK выбранной базовой версии; KeepVersion повторяет последнюю локальную версию.
Для одной папки distribution сборки запускаются последовательно: распределённого
резервирования номеров между несколькими процессами этот механизм не выполняет.

## Публикация

Изменить версию в AndroidBuildTools.nuspec и выполнить `Pack.ps1 -FeedPath C:\NugetFeed`
или `Scripts/Nuget.AndroidBuildTools.Pack.cmd` из UtilityHelpersLib.
Скрипт запрещает заменять уже опубликованную версию. Затем явно обновить
BuildToolsVersion в конфигурации приложения и проверить сборку.

Пакет содержит инструменты, но не устанавливает SDK/NDK/JDK и не изменяет applicationId,
подпись, зависимости Gradle или исходники приложения. Интеграция конкретных
native-зависимостей остаётся в приложении.