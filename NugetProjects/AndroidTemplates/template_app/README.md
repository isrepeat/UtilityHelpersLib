# {{Application}}

```powershell
./build.ps1 build-android -Configuration Debug
./build.ps1 build-and-distribute -Destination Local -Configuration Debug
```

Проект создан пакетом `AndroidBuildTools` версии `{{AndroidBuildToolsVersion}}`. `android-build.psd1` содержит пути к пакетам
и инструментам, `{{Application}}.Android/build.gradle.kts` — package ID и Android-ресурсы,
`{{Application}}.AndroidHost` — native-код. Gradle wrapper включён в репозиторий.
`{{Application}}.Application/UI/Page/MainPage.xaml` — минимальная нативная страница;
её `.cpp/.h` генерирует `XamlCompiler` в `!Generated` перед сборкой CMake.

При генерации проекта в `android-build.psd1` уже записан путь `SigningProperties`
к общему файлу `SecretsRoot/shared/signing.properties` вне Git. Все приложения и
ApkUpdater используют его Debug и Release ключи. Генератор создаёт их один раз.
Для CI этот путь можно
переопределить `ANDROID_SIGNING_PROPERTIES`.

Конфигурация сразу содержит секции `Xaml`, `Preview` и `Drive`, как у полноценного
приложения. `Preview.Executable.Debug` и `Preview.Executable.Release` содержат
пути к готовому AndroidAppPreviewer, а Drive использует пути к секретам,
переданные `New-AndroidApplication.ps1`.


## XAML-preview

Соберите только `PreviewPlugin.dll`, не запуская AndroidAppPreviewer:

```powershell
$projectRoot = 'C:\WORK\Android\Projects\{{Application}}'
Set-Location $projectRoot
.\build.ps1 run-android-app-previewer -BuildOnly -Configuration Debug
```

Для сборки плагина и запуска previewer выполните:

```powershell
$projectRoot = 'C:\WORK\Android\Projects\{{Application}}'
Set-Location $projectRoot
.\build.ps1 run-android-app-previewer -Configuration Debug
```

Пути к `AndroidAppPreviewer.exe` для Debug и Release находятся в параметрах
`Preview.Executable.Debug` и `Preview.Executable.Release` файла `android-build.psd1`.

## Логирование Android

Кнопка `Send logs` после `Update` отправляет журнал текущего сеанса в Google Drive,
в папку `Android/{{Application}}` рядом с APK. Результат отображается на главной
странице; при необходимости Google запрашивает доступ к Drive.

При запуске `MainActivity` создаётся журнал сеанса в
`Downloads/com.isrepeat/{{Application}}`. Kotlin передаёт открытый file descriptor
в native host через `NativeSessionLog` из AndroidAppKit. Android host настраивает
`Helpers.Logging` на этот файл, поэтому записи `LOG_INFO`, `LOG_WARNING` и
`LOG_ERROR` из общей C++ библиотеки попадают в журнал. PreviewPlugin настраивает
тот же logger через `xp_configure_logging`, поэтому эти записи также видны в
журнале previewer. Имя Android-журнала содержит дату и время запуска; перед
уничтожением native-сессии logger принудительно сбрасывается на диск.

## Update из Google Drive

Стартовый `MainPage` содержит кнопку **Update**. Она вызывает
`androidappkit.update.GoogleDriveUpdateController`, который ищет versioned APK в
`Android/{{Application}}` на Google Drive. Кнопка использует Android OAuth client
этого package ID и сертификата подписи.

Команда проходит по цепочке: `RequestApplicationUpdateCommand` →
`AppSessionController` → `AndroidCommandDispatcher` (JNI) →
`NativeCommandDispatcher` (UI-поток) → `GoogleDriveUpdateController.start()`.
`ApplicationSession` владеет контроллером; `PageContext.hostCommands` предоставляет
страницам интерфейс отправки команд. Числовые IDs `HostCommand` в C++ и Kotlin
должны совпадать. В JNI строки передаются как UTF-8 byte arrays.

Статус проверки и загрузки возвращается через `MainPage.setStatus()` и очередь
GL-потока в `AppSessionController`, затем в XAML binding `Status`. Операции с C++ UI
выполняются на GL-потоке. При уничтожении Activity отключается Kotlin handler,
после обработки очереди удаляется native-сессия и освобождается JNI global reference.
В preview Update показывает сообщение о доступности обновления в Android host.

Контроллер из AndroidAppKit обрабатывает авторизацию, повторный запуск, загрузку,
проверку package ID и версии. Для установленной версии host запрашивает подтверждение
переустановки. Передача APK в ApkUpdater ещё не означает успешную установку.

Для установки найденного APK нужен установленный `ApkUpdater`. Он должен быть
подписан тем же Debug или Release сертификатом, что и приложение, а его опубликованный
список разрешённых пакетов должен содержать `{{PackageId}}`. Секреты Desktop OAuth
для загрузки APK остаются вне Git и добавляются в секцию `Drive` проекта, когда
нужна команда `build-and-distribute -Destination Drive`.

## Удаление тестового проекта

Удаление выполняет скрипт, скопированный в проект генератором:

```powershell
& ".\Scripts\PowerShell\Remove-AndroidProject.ps1" -ProjectRoot (Get-Location)
```

Скрипт запросит подтверждение `Y/N`, после чего удалит корень
проекта и связанный каталог secrets.