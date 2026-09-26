package com.isrepeat.build

import org.gradle.api.GradleException
import org.gradle.api.JavaVersion
import org.gradle.api.Plugin
import org.gradle.api.Project

class AndroidApplicationPlugin implements Plugin<Project> {
    void apply(Project project) {
        project.pluginManager.apply('com.android.application')
        def repositoryRoot = project.rootProject.projectDir.parentFile.parentFile
        project.layout.buildDirectory.set(new File(repositoryRoot, "Build/${project.name}"))
        def versionFile = project.providers.gradleProperty('appVersionFile')
            .map { new File(repositoryRoot, it) }.getOrElse(new File(repositoryRoot, 'version.properties'))
        def versions = new Properties()
        if (versionFile.isFile()) {
            versions.load(new StringReader(project.providers.fileContents(project.layout.projectDirectory.file(versionFile.absolutePath)).asText.get()))
        }
        def code = project.providers.gradleProperty('appVersionCode').orNull
        def name = project.providers.gradleProperty('appVersionName').orNull
        if ((code == null) != (name == null)) {
            throw new GradleException('appVersionCode and appVersionName must be specified together.')
        }
        code = code ?: versions.getProperty('VERSION_CODE_BASE')
        name = name ?: versions.getProperty('VERSION_NAME_BASE')?.concat('.0')
        if (code == null || name == null || !(code ==~ /[1-9][0-9]*/) || code.toLong() > 2100000000L) {
            throw new GradleException('Provide valid appVersionCode/appVersionName or VERSION_CODE_BASE/VERSION_NAME_BASE.')
        }
        def signingPath = project.providers.gradleProperty('androidSigningProperties')
            .orElse(project.providers.environmentVariable('ANDROID_SIGNING_PROPERTIES')).orNull
        def signing = new Properties()
        if (signingPath != null && project.file(signingPath).isFile()) {
            def signingFile = project.layout.projectDirectory.file(project.file(signingPath).absolutePath)
            signing.load(new StringReader(project.providers.fileContents(signingFile).asText.get()))
            ['storeFile', 'storePassword', 'keyAlias', 'keyPassword'].each {
                if (!signing.getProperty(it)) {
                    throw new GradleException("Signing properties must define ${it}.")
                }
            }
        }
        project.extensions.configure(com.android.build.api.dsl.ApplicationExtension) { android ->
            android.compileSdk = 36
            android.defaultConfig {
                minSdk = 24
                targetSdk = 36
                versionCode = code.toInteger()
                versionName = name
                ndk.abiFilters.add('arm64-v8a')
            }
            android.compileOptions {
                sourceCompatibility = JavaVersion.VERSION_11
                targetCompatibility = JavaVersion.VERSION_11
            }
            // В AGP 9 встроенный Kotlin наследует JVM target из compileOptions.
            android.buildTypes.getByName('release').optimization.enable = false
            if (!signing.isEmpty()) {
                def releaseSigning = android.signingConfigs.maybeCreate('release')
                releaseSigning.storeFile = project.file(signing.getProperty('storeFile'))
                releaseSigning.storePassword = signing.getProperty('storePassword')
                releaseSigning.keyAlias = signing.getProperty('keyAlias')
                releaseSigning.keyPassword = signing.getProperty('keyPassword')
                android.buildTypes.getByName('release').signingConfig = releaseSigning
            }
            // Проверка выполняется перед сборкой release, поэтому debug доступен без ключа.
            def verifySigning = project.tasks.register('verifyReleaseSigning', VerifyReleaseSigning)
            project.extensions.getByName('androidComponents').finalizeDsl { finalized ->
                verifySigning.configure {
                    configured.set(finalized.buildTypes.getByName('release').signingConfig?.storeFile?.isFile() == true)
                }
            }
            project.tasks.matching { it.name == 'preReleaseBuild' }.configureEach {
                dependsOn(verifySigning)
            }
            def nativeDirectory = project.providers.gradleProperty('appNativeLibraries').orNull
            if (nativeDirectory != null) {
                android.sourceSets.getByName('main').jniLibs.srcDir(new File(repositoryRoot, nativeDirectory))
            }
        }
        project.dependencies.add('implementation', 'androidx.activity:activity-ktx:1.8.0')
        project.dependencies.add('implementation', 'androidx.lifecycle:lifecycle-runtime-ktx:2.6.1')
        project.configurations.configureEach {
            resolutionStrategy.cacheDynamicVersionsFor(0, 'seconds')
        }
    }
}