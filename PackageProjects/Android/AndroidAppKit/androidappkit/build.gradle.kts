plugins {
    id("com.android.library")
    id("maven-publish")
}

group = providers.gradleProperty("packageGroup").get()
version = providers.gradleProperty("packageVersion")
    .orElse(providers.gradleProperty("packageVersionBase").map { "$it.0" })
    .get()

android {
    namespace = "com.isrepeat.androidappkit"
    compileSdk = providers.gradleProperty("androidCompileSdk").get().toInt()

    defaultConfig {
        minSdk = providers.gradleProperty("androidMinSdk").get().toInt()
    }

    publishing {
        singleVariant("release") {
            withSourcesJar()
        }
    }
}

dependencies {
    api("com.isrepeat:androidcoresdk:+")
}

publishing {
    publications {
        register<MavenPublication>("release") {
            groupId = project.group.toString()
            artifactId = "androidappkit"
            version = project.version.toString()

            afterEvaluate {
                from(components["release"])
            }
        }
    }

    repositories {
        maven {
            name = "AndroidPackagesFeed"
            url = uri(providers.gradleProperty("androidPackagesFeedPath").get())
        }
    }
}