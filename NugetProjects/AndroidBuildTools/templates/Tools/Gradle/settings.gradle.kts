pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        maven {
            url = uri(System.getenv("ANDROID_PACKAGES_FEED") ?: "C:/!PackagesFeed/Android")
        }
        google()
        mavenCentral()
    }
}

rootProject.name = "<Application>"
include(":<Application>.Android")
project(":<Application>.Android").projectDir = file("../../<Application>.Android")

gradle.beforeProject {
    layout.buildDirectory.set(rootProject.file("../../Build/$name"))
}