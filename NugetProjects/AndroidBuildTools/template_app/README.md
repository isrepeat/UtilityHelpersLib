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