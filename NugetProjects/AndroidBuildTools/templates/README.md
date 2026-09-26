# <Application>

```powershell
./build.ps1 build-android -Configuration Debug
./build.ps1 build-and-distribute -Destination Local -Configuration Debug
```

Проект создан из AndroidBuildTools. `android-build.psd1` содержит версию пакета
и пути, `<Application>.Android/build.gradle.kts` — package ID и Android-ресурсы,
`<Application>.AndroidHost` — native-код. Gradle wrapper включён в репозиторий.

Для Release задайте `ANDROID_SIGNING_PROPERTIES` — путь к внешнему файлу с
`storeFile`, `storePassword`, `keyAlias`, `keyPassword`.

XAML, desktop previewer и Drive подключаются дополнительными секциями конфигурации;
их исходники и секреты не входят в минимальный проект.