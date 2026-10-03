package {{PackageId}}

import com.isrepeat.androidappkit.androidappkit

class MainActivity : androidx.activity.ComponentActivity() {
    private lateinit var mainPage: MainPage
    private lateinit var dispatcher: NativeCommandDispatcher
    private lateinit var sessionLog: androidappkit.diagnostics.NativeSessionLog
    private lateinit var updateController: androidappkit.update.GoogleDriveUpdateController
    private lateinit var logsUploader: androidappkit.drive.GoogleDriveUploader
    private var reinstallConfirmation: android.app.AlertDialog? = null

    private val authorizeUpdate = registerForActivityResult(
        androidx.activity.result.contract.ActivityResultContracts.StartIntentSenderForResult(),
    ) { updateController.completeAuthorization(it.data) }

    private val authorizeLogs = registerForActivityResult(
        androidx.activity.result.contract.ActivityResultContracts.StartIntentSenderForResult(),
    ) { logsUploader.completeAuthorization(it.data) }

    override fun onCreate(savedInstanceState: android.os.Bundle?) {
        super.onCreate(savedInstanceState)
        dispatcher = NativeCommandDispatcher { command, _, _ ->
            when (command) {
                HostCommand.REQUEST_APPLICATION_UPDATE -> updateController.start()
                HostCommand.SEND_LOGS -> sendLogs()
            }
        }
        mainPage = MainPage(this, dispatcher)
        sessionLog = androidappkit.diagnostics.NativeSessionLog(
            androidappkit.AppIdentity("{{Application}}", "com.isrepeat/{{Application}}"),
            androidappkit.NativeLogConfigurator { nativePath -> mainPage.configureNativeLog(nativePath) },
        )
        sessionLog.configure(this)
        androidappkit.diagnostics.configureLogger(
            androidappkit.diagnostics.AppKitLogger { message -> NativeDiagnostics.log(message) },
        )
        val driveFolder = listOf("Android", "{{Application}}")
        logsUploader = androidappkit.drive.GoogleDriveUploader(
            this,
            androidappkit.drive.GoogleDriveUploadConfiguration(driveFolder),
            authorizeLogs::launch,
            { result ->
                if (!isDestroyed) {
                    mainPage.setStatus(when (result) {
                        is androidappkit.drive.GoogleDriveUploadResult.Success -> "Logs sent: ${result.fileName}"
                        is androidappkit.drive.GoogleDriveUploadResult.Failure -> result.message
                    })
                }
            },
        )
        updateController = androidappkit.update.GoogleDriveUpdateController(
            this,
            androidappkit.update.GoogleDriveUpdateConfiguration(
                driveFolder,
                Regex("{{Application}}-(\\d+)\\.(\\d+)\\.(\\d+)(?:-debug|-release)\\.apk", RegexOption.IGNORE_CASE),
                { match ->
                    match.groupValues[1].toLong() * 1_000_000L +
                        match.groupValues[2].toLong() * 1_000L +
                        match.groupValues[3].toLong()
                },
                "com.isrepeat.apkupdater",
                "com.isrepeat.apkupdater.UpdaterActivity",
                "com.isrepeat.apkupdater.permission.INSTALL_UPDATE",
                "com.isrepeat.apkupdater.action.INSTALL_UPDATE",
                confirmSameVersionInUpdater = true,
            ),
            authorizeUpdate::launch,
            { message ->
                if (!isDestroyed) {
                    mainPage.setStatus(message)
                }
            },
            androidappkit.diagnostics.sharedLogger(),
            { onConfirmed, onCancelled ->
                // Для старого ApkUpdater подтверждение остаётся в приложении.
                reinstallConfirmation = android.app.AlertDialog.Builder(this)
                    .setTitle("Reinstall application?")
                    .setMessage("This version is already installed.")
                    .setPositiveButton("Reinstall") { _, _ -> onConfirmed() }
                    .setNegativeButton("Cancel") { _, _ -> onCancelled() }
                    .setOnCancelListener { onCancelled() }
                    .show()
            },
        )
        setContentView(mainPage)
        recordUpdaterResult(intent)
        onBackPressedDispatcher.addCallback(this, object : androidx.activity.OnBackPressedCallback(true) {
            override fun handleOnBackPressed() {
                mainPage.navigateBack { finish() }
            }
        })
    }

    override fun onNewIntent(intent: android.content.Intent) {
        super.onNewIntent(intent)
        setIntent(intent)
        recordUpdaterResult(intent)
    }

    private fun recordUpdaterResult(intent: android.content.Intent) {
        // Диагностика updater попадает в тот же журнал, который отправляет Send logs.
        intent.getStringExtra("updater_trace")?.takeIf { it.isNotBlank() }?.let(NativeDiagnostics::log)
        intent.getStringExtra("update_error")?.let(mainPage::setStatus)
        if (intent.action == "com.isrepeat.apkupdater.action.UPDATE_COMPLETED") {
            mainPage.setStatus("Update installed.")
        }
    }

    private fun sendLogs() {
        val uri = sessionLog.currentUri()
        if (uri == null) {
            mainPage.setStatus("The session log is not available.")
            return
        }
        mainPage.setStatus("Sending logs to Google Drive…")
        logsUploader.upload(uri, "text/plain", "{{Application}}-session-${System.currentTimeMillis()}.log")
    }

    override fun onResume() {
        super.onResume()
        mainPage.onResume()
    }

    override fun onPause() {
        mainPage.pauseSession()
        super.onPause()
    }

    override fun onDestroy() {
        dispatcher.close()
        reinstallConfirmation?.dismiss()
        mainPage.destroySession()
        super.onDestroy()
    }
}