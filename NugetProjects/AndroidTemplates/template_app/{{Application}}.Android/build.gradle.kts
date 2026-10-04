plugins {
    id("com.isrepeat.android.application")
}

android {
    namespace = "{{PackageId}}"
    defaultConfig.applicationId = "{{PackageId}}"
    // Отображаемое имя каждой сборки задаётся самим приложением.
    buildTypes {
        getByName("debug") {
            manifestPlaceholders["appTitle"] = "{{Application}} ${defaultConfig.versionName} (debug)"
        }
        getByName("release") {
            manifestPlaceholders["appTitle"] = "{{Application}}"
        }
    }
    sourceSets.getByName("main").jniLibs.srcDir("../Build/{{Application}}.AndroidHost/android/jniLibs")
}

dependencies {
    implementation("com.isrepeat:androidappkit:1.0.17")
    implementation("com.google.android.gms:play-services-auth:21.3.0")
}