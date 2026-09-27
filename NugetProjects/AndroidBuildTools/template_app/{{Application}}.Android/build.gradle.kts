plugins {
    id("com.isrepeat.android.application")
}

android {
    namespace = "{{PackageId}}"
    defaultConfig.applicationId = "{{PackageId}}"
    sourceSets.getByName("main").jniLibs.srcDir("../Build/{{Application}}.AndroidHost/android/jniLibs")
}

dependencies {
    implementation("com.isrepeat:androidappkit:+")
    implementation("com.google.android.gms:play-services-auth:21.3.0")
}