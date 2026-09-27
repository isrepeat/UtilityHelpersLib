plugins {
    id("com.isrepeat.android.application")
}

android {
    namespace = "{{PackageId}}"
    defaultConfig.applicationId = "{{PackageId}}"
    sourceSets.getByName("main").jniLibs.srcDir("../Build/{{Application}}.AndroidHost/android/jniLibs")
}