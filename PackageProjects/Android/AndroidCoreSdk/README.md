# AndroidCoreSdk

Минимальная Android AAR-библиотека с Maven-координатами `com.isrepeat:androidcoresdk:+`.

## Первый запуск

В каталоге проекта установите JDK 17 и Gradle, затем один раз создайте Gradle Wrapper:

```powershell
gradle wrapper --gradle-version 8.11.1
```

## Сборка и публикация

```powershell
.\Package.Android.AndroidCoreSdk.Pack.ps1
```

Скрипт увеличивает patch-часть `packageVersion`, собирает AAR в
`androidcoresdk\build\outputs\aar\androidcoresdk-release.aar` и публикует Maven-пакет в
`C:\!PackagesFeed\Android`. В потребляющем проекте подключите этот каталог как Maven-репозиторий
и добавьте зависимость `com.isrepeat:androidcoresdk:+`, чтобы получать последнюю версию.