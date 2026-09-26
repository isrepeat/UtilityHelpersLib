import java.util.Properties

val repositoryRoot = rootProject.projectDir.parentFile.parentFile
val versionProperties = Properties().apply {
    repositoryRoot.resolve("version.properties").inputStream().use(this::load)
}
val appVersionCode = providers.gradleProperty("appVersionCode")
    .orElse(versionProperties.getProperty("VERSION_CODE_BASE"))
    .get().toInt()
val appVersionName = providers.gradleProperty("appVersionName")
    .orElse(versionProperties.getProperty("VERSION_NAME_BASE"))
    .get()

plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.isrepeat.<application>"
    compileSdk {
        version = release(36)
    }
    defaultConfig {
        applicationId = "com.isrepeat.<application>"
        minSdk = 24
        targetSdk = 36
        versionCode = appVersionCode
        versionName = appVersionName
        ndk {
            abiFilters += "arm64-v8a"
        }
    }
    sourceSets {
        getByName("main").assets.directories += "../<Application>.Application/Resources"
        getByName("main").jniLibs.srcDirs("../Build/<Application>.AndroidHost/android/jniLibs")
    }
}

dependencies {
    implementation("com.isrepeat:androidappkit:+")
    implementation("com.isrepeat:androidcoresdk:+")
    implementation(libs.androidx.activity)
    implementation(libs.androidx.lifecycle)
}

configurations.configureEach {
    resolutionStrategy.cacheDynamicVersionsFor(0, "seconds")
}