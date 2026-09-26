package com.isrepeat.build

import org.gradle.api.Plugin
import org.gradle.api.initialization.Settings
import org.gradle.api.initialization.resolve.RepositoriesMode

class AndroidSettingsPlugin implements Plugin<Settings> {
    void apply(Settings settings) {
        settings.pluginManagement.repositories {
            google()
            mavenCentral()
            gradlePluginPortal()
        }
        settings.dependencyResolutionManagement {
            repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
            repositories {
                maven {
                    url = settings.providers.gradleProperty('androidMavenSource')
                        .orElse(settings.providers.environmentVariable('ANDROID_MAVEN_SOURCE'))
                        .orElse('C:/!PackagesFeed/Android').get()
                }
                google()
                mavenCentral()
            }
        }
    }
}