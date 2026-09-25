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
        maven { url = uri("C:/!PackagesFeed/Android") }
        google()
        mavenCentral()
    }
}

rootProject.name = "AndroidAppKit"
include(":androidappkit")