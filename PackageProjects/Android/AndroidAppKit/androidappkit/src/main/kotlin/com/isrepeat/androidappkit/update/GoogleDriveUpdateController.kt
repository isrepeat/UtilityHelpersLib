package com.isrepeat.androidappkit.update

// Ищет APK на Google Drive, проверяет его и передаёт доверенному updater-приложению.
class GoogleDriveUpdateController(
    private val activity: androidx.activity.ComponentActivity,
    private val configuration: GoogleDriveUpdateConfiguration,
    private val requestAuthorization: (androidx.activity.result.IntentSenderRequest) -> Unit,
    private val status: (String) -> Unit,
    private val logger: UpdateLogger,
    private val confirmSameVersion: (onConfirmed: () -> Unit, onCancelled: () -> Unit) -> Unit =
        { onConfirmed, _ -> onConfirmed() },
) {
    private var isRunning = false

    fun start() {
        if (isRunning) return status("An update is already in progress.")
        isRunning = true
        status("Checking Google Drive access…")
        val request = com.google.android.gms.auth.api.identity.AuthorizationRequest.builder()
            .setRequestedScopes(listOf(com.google.android.gms.common.api.Scope(DRIVE_SCOPE)))
            .build()
        com.google.android.gms.auth.api.identity.Identity.getAuthorizationClient(activity).authorize(request)
            .addOnSuccessListener(::handleAuthorization)
            .addOnFailureListener { finish("Google Drive access was not granted: ${it.message}") }
    }

    fun completeAuthorization(intent: android.content.Intent?) {
        runCatching {
            com.google.android.gms.auth.api.identity.Identity.getAuthorizationClient(activity)
                .getAuthorizationResultFromIntent(intent)
        }.onSuccess(::handleAuthorization)
            .onFailure { finish("Google Drive access was not granted: ${it.message}") }
    }

    private fun handleAuthorization(result: com.google.android.gms.auth.api.identity.AuthorizationResult) {
        if (result.hasResolution()) {
            val pendingIntent = result.pendingIntent ?: return finish("Google did not provide an authorization screen.")
            requestAuthorization(androidx.activity.result.IntentSenderRequest.Builder(pendingIntent.intentSender).build())
            return
        }
        val token = result.accessToken ?: return finish("Google did not provide a Drive access token.")
        com.isrepeat.androidcoresdk.androidcoresdk.coroutines.LifecycleCoroutineRunner.launch(activity) {
            runCatching { kotlinx.coroutines.withContext(kotlinx.coroutines.Dispatchers.IO) { download(token) } }
                .onSuccess { apk -> if (apk == null) finish("No new version was found on Google Drive.") else confirmOrLaunch(apk) }
                .onFailure { finish("Failed to update the application: ${it.message}") }
        }
    }

    private fun download(token: String): java.io.File? {
        val client = com.isrepeat.androidcoresdk.androidcoresdk.drive.GoogleDriveClient(token)
        val candidate = client.listFiles(client.ensureFolderPath(configuration.driveFolderPath))
            .mapNotNull { file -> configuration.apkNamePattern.matchEntire(file.name)?.groupValues?.get(1)?.toLongOrNull()?.let { file to it } }
            .maxByOrNull { it.second } ?: return null
        val apk = java.io.File(activity.cacheDir, "self-updates/update.apk").apply { parentFile?.mkdirs() }
        try {
            client.download(candidate.first.id, apk)
            validateApk(apk)
            return apk
        } catch (exception: Exception) {
            apk.delete()
            throw exception
        }
    }

    private fun validateApk(apk: java.io.File) {
        val archive = activity.packageManager.getPackageArchiveInfo(apk.path, 0)
            ?: error("Google Drive returned an invalid APK.")
        check(archive.packageName == activity.packageName) { "The APK targets ${archive.packageName}, not ${activity.packageName}." }
        check(versionCode(archive) >= installedVersionCode()) { "Google Drive contains only an older version." }
    }

    private fun confirmOrLaunch(apk: java.io.File) {
        val version = activity.packageManager.getPackageArchiveInfo(apk.path, 0)?.let(::versionCode)
            ?: return finish("Failed to read the downloaded APK version.")
        if (version != installedVersionCode()) return launchUpdaterAndFinish(apk)
        confirmSameVersion({ launchUpdaterAndFinish(apk) }, { finish("Reinstallation was cancelled.") })
    }

    private fun launchUpdaterAndFinish(apk: java.io.File) {
        runCatching {
        val pm = activity.packageManager
        check(pm.checkSignatures(activity.packageName, configuration.updaterPackage) == android.content.pm.PackageManager.SIGNATURE_MATCH) { "The application and updater are signed with different keys." }
        check(pm.checkPermission(configuration.updaterPermission, activity.packageName) == android.content.pm.PackageManager.PERMISSION_GRANTED) { "Android did not grant permission to invoke the updater application." }
        val uri = androidx.core.content.FileProvider.getUriForFile(activity, "${activity.packageName}.fileprovider", apk)
        activity.startActivity(android.content.Intent(configuration.updaterAction)
            .setClassName(configuration.updaterPackage, configuration.updaterActivity)
            .setDataAndType(uri, APK_MIME_TYPE)
            .addFlags(android.content.Intent.FLAG_ACTIVITY_NEW_TASK or android.content.Intent.FLAG_GRANT_READ_URI_PERMISSION)
            .putExtra(configuration.targetPackageExtra, activity.packageName))
        logger.log("Update passed to the updater application: ${apk.length()} bytes")
        }.onSuccess { finish("Update passed to the updater application. Waiting for installation…") }
            .onFailure { finish("Failed to start the updater application: ${it.message}") }
    }

    private fun finish(message: String) { isRunning = false; status(message) }

    @Suppress("DEPRECATION")
    private fun versionCode(info: android.content.pm.PackageInfo): Long =
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.P) info.longVersionCode else info.versionCode.toLong()

    @Suppress("DEPRECATION")
    private fun installedVersionCode(): Long = versionCode(activity.packageManager.getPackageInfo(activity.packageName, 0))

    private companion object {
        const val APK_MIME_TYPE = "application/vnd.android.package-archive"
        const val DRIVE_SCOPE = "https://www.googleapis.com/auth/drive"
    }
}