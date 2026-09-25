package com.isrepeat.androidappkit

// Псевдопространства имён и типизированные фасады для публичного API AppKit.
object androidappkit {
    data class AppIdentity(val name: String, val storageDirectory: String)

    fun interface NativeLogConfigurator {
        fun configure(nativePath: String)
    }

    object drive {
        data class GoogleDriveUploadConfiguration(val folderPath: List<String>)

        sealed interface GoogleDriveUploadResult {
            data class Success(val fileName: String) : GoogleDriveUploadResult
            data class Failure(val message: String) : GoogleDriveUploadResult
        }

        class GoogleDriveUploader(
            activity: androidx.activity.ComponentActivity,
            configuration: GoogleDriveUploadConfiguration,
            requestAuthorization: (androidx.activity.result.IntentSenderRequest) -> Unit,
            complete: (GoogleDriveUploadResult) -> Unit,
        ) {
            private val uploader = com.isrepeat.androidappkit.drive.GoogleDriveUploader(
                activity,
                com.isrepeat.androidappkit.drive.GoogleDriveUploadConfiguration(configuration.folderPath),
                requestAuthorization,
            ) { result ->
                complete(
                    when (result) {
                        is com.isrepeat.androidappkit.drive.GoogleDriveUploadResult.Success -> GoogleDriveUploadResult.Success(result.fileName)
                        is com.isrepeat.androidappkit.drive.GoogleDriveUploadResult.Failure -> GoogleDriveUploadResult.Failure(result.message)
                    },
                )
            }

            fun upload(file: java.io.File, mimeType: String = "application/octet-stream") = uploader.upload(file, mimeType)
            fun upload(uri: android.net.Uri, mimeType: String? = null, fileName: String? = null) = uploader.upload(uri, mimeType, fileName)
            fun completeAuthorization(intent: android.content.Intent?) = uploader.completeAuthorization(intent)
        }
    }

    object logging {
        class NativeSessionLog(identity: AppIdentity, configureNativeLog: NativeLogConfigurator) {
            private val log = com.isrepeat.androidappkit.logging.NativeSessionLog(
                com.isrepeat.androidappkit.AppIdentity(identity.name, identity.storageDirectory),
            ) { configureNativeLog.configure(it) }

            fun configure(context: android.content.Context) = log.configure(context)
            fun currentUri() = log.currentUri()
        }
    }

    object media {
        fun SurfaceScreenshotCapture(directoryName: String, filePrefix: String) =
            com.isrepeat.androidappkit.media.SurfaceScreenshotCapture(directoryName, filePrefix)
    }

    object update {
        data class GoogleDriveUpdateConfiguration(
            val driveFolderPath: List<String>,
            val apkNamePattern: Regex,
            val updaterPackage: String,
            val updaterActivity: String,
            val updaterPermission: String,
            val updaterAction: String,
        )

        fun interface UpdateLogger {
            fun log(message: String)
        }

        class GoogleDriveUpdateController(
            activity: androidx.activity.ComponentActivity,
            configuration: GoogleDriveUpdateConfiguration,
            requestAuthorization: (androidx.activity.result.IntentSenderRequest) -> Unit,
            status: (String) -> Unit,
            logger: UpdateLogger,
            confirmSameVersion: (onConfirmed: () -> Unit, onCancelled: () -> Unit) -> Unit,
        ) {
            private val controller = com.isrepeat.androidappkit.update.GoogleDriveUpdateController(
                activity,
                com.isrepeat.androidappkit.update.GoogleDriveUpdateConfiguration(
                    configuration.driveFolderPath,
                    configuration.apkNamePattern,
                    configuration.updaterPackage,
                    configuration.updaterActivity,
                    configuration.updaterPermission,
                    configuration.updaterAction,
                ),
                requestAuthorization,
                status,
                com.isrepeat.androidappkit.update.UpdateLogger { logger.log(it) },
                confirmSameVersion,
            )

            fun start() = controller.start()
            fun completeAuthorization(intent: android.content.Intent?) = controller.completeAuthorization(intent)
        }
    }
}