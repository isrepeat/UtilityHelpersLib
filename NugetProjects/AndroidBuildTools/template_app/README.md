# {{Application}}

```powershell
./build.ps1 build-android -Configuration Debug
./build.ps1 build-and-distribute -Destination Local -Configuration Debug
```

Проект создан из AndroidBuildTools. `android-build.psd1` содержит версию пакета
и пути, `{{Application}}.Android/build.gradle.kts` — package ID и Android-ресурсы,
`{{Application}}.AndroidHost` — native-код. Gradle wrapper включён в репозиторий.

При генерации проекта в `android-build.psd1` уже записан путь `SigningProperties`
к секретам вне Git. Он содержит ключи Debug и Release. Для CI этот путь можно
переопределить `ANDROID_SIGNING_PROPERTIES`.

XAML, desktop previewer и Drive подключаются дополнительными секциями конфигурации;
их исходники и секреты не входят в минимальный проект.

## Update из Google Drive

Стартовый `MainPage` содержит кнопку **Update**. Она вызывает
`androidappkit.update.GoogleDriveUpdateController`, который ищет versioned APK в
`Android/{{Application}}` на Google Drive. Кнопка использует Android OAuth client
этого package ID и сертификата подписи.

Для установки найденного APK нужен установленный `ApkUpdater`. Он должен быть
подписан тем же Debug или Release сертификатом, что и приложение, а его опубликованный
список разрешённых пакетов должен содержать `{{PackageId}}`. Секреты Desktop OAuth
для загрузки APK остаются вне Git и добавляются в секцию `Drive` проекта, когда
нужна команда `build-and-distribute -Destination Drive`.

## Удаление тестового проекта

Удаление выполняет скрипт из пакета AndroidBuildTools:

```powershell
$tools = .\build.ps1 restore
& "$tools\tools\Remove-AndroidProject.ps1" -ProjectRoot (Get-Location)
```

Он сам запросит package ID и подтверждение `Y/N`. Для автоматического сценария
можно передать package ID явно: `-PackageId {{PackageId}}`.

Скрипт сверяет package ID с каталогом из `SigningProperties`, при необходимости
сам переносит выполнение во временный каталог, затем удаляет корень проекта и
связанный каталог secrets. Используйте его только для проекта, который больше не
нужен.