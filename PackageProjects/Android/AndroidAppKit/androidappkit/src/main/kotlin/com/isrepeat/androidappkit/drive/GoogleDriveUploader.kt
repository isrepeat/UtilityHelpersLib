package com.isrepeat.androidappkit.drive

data class GoogleDriveUploadConfiguration(val folderPath: List<String>)

sealed interface GoogleDriveUploadResult {
    data class Success(val fileName: String) : GoogleDriveUploadResult
    data class Failure(val message: String) : GoogleDriveUploadResult
}

class GoogleDriveUploader(
    private val activity: androidx.activity.ComponentActivity,
    private val configuration: GoogleDriveUploadConfiguration,
    private val requestAuthorization: (androidx.activity.result.IntentSenderRequest) -> Unit,
    private val complete: (GoogleDriveUploadResult) -> Unit,
) {
    private data class PendingFile(val name: String, val mimeType: String, val open: () -> java.io.InputStream?)

    private var pendingFile: PendingFile? = null
    private var isRunning = false

    fun upload(file: java.io.File, mimeType: String = "application/octet-stream") {
        if (!file.isFile) {
            complete(GoogleDriveUploadResult.Failure("File ${file.name} was not found."))
            return
        }
        upload(PendingFile(file.name, mimeType, file::inputStream))
    }

    fun upload(uri: android.net.Uri, mimeType: String? = null, fileName: String? = null) {
        val name = fileName ?: uri.lastPathSegment?.substringAfterLast('/') ?: "upload"
        val type = mimeType ?: activity.contentResolver.getType(uri) ?: "application/octet-stream"
        upload(PendingFile(name, type) { activity.contentResolver.openInputStream(uri) })
    }

    fun completeAuthorization(intent: android.content.Intent?) {
        runCatching {
            com.google.android.gms.auth.api.identity.Identity
                .getAuthorizationClient(activity)
                .getAuthorizationResultFromIntent(intent)
        }
            .onSuccess(::handleAuthorization)
            .onFailure { finish(GoogleDriveUploadResult.Failure("Google Drive access was not granted: ${it.message}")) }
    }

    private fun upload(file: PendingFile) {
        if (isRunning) {
            complete(GoogleDriveUploadResult.Failure("A Google Drive upload is already in progress."))
            return
        }
        isRunning = true
        pendingFile = file
        val request = com.google.android.gms.auth.api.identity.AuthorizationRequest.builder()
            .setRequestedScopes(listOf(com.google.android.gms.common.api.Scope(DRIVE_SCOPE)))
            .build()
        com.google.android.gms.auth.api.identity.Identity.getAuthorizationClient(activity).authorize(request)
            .addOnSuccessListener(::handleAuthorization)
            .addOnFailureListener { finish(GoogleDriveUploadResult.Failure("Google Drive access was not granted: ${it.message}")) }
    }

    private fun handleAuthorization(result: com.google.android.gms.auth.api.identity.AuthorizationResult) {
        if (result.hasResolution()) {
            val pendingIntent = result.pendingIntent
                ?: return finish(GoogleDriveUploadResult.Failure("Google did not provide an authorization screen."))
            requestAuthorization(
                androidx.activity.result.IntentSenderRequest.Builder(pendingIntent.intentSender).build(),
            )
            return
        }
        val token = result.accessToken ?: return finish(GoogleDriveUploadResult.Failure("Google did not provide a Drive access token."))
        val file = pendingFile ?: return finish(GoogleDriveUploadResult.Failure("No file was selected for upload."))
        com.isrepeat.androidcoresdk.androidcoresdk.coroutines.LifecycleCoroutineRunner.launch(activity) {
            val result = runCatching {
                kotlinx.coroutines.withContext(kotlinx.coroutines.Dispatchers.IO) {
                    com.isrepeat.androidcoresdk.androidcoresdk.drive.GoogleDriveClient(token).run {
                        upload(ensureFolderPath(configuration.folderPath), file.name, file.mimeType, file.open)
                    }
                }
            }.fold(
                onSuccess = { GoogleDriveUploadResult.Success(file.name) },
                onFailure = { GoogleDriveUploadResult.Failure("Failed to upload ${file.name}: ${it.message}") },
            )
            finish(result)
        }
    }

    private fun finish(result: GoogleDriveUploadResult) {
        pendingFile = null
        isRunning = false
        complete(result)
    }

    private companion object {
        const val DRIVE_SCOPE = "https://www.googleapis.com/auth/drive"
    }
}