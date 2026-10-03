package {{PackageId}}

import com.isrepeat.androidappkit.androidappkit

class MainActivity : androidx.activity.ComponentActivity() {
    private lateinit var mainPage: MainPage
    private lateinit var dispatcher: NativeCommandDispatcher
    private lateinit var sessionLog: androidappkit.diagnostics.NativeSessionLog
    private lateinit var updateController: androidappkit.update.GoogleDriveUpdateController
    private var confirmation: android.app.AlertDialog? = null

    private val authorizeUpdate = registerForActivityResult(
        androidx.activity.result.contract.ActivityResultContracts.StartIntentSenderForResult(),
    ) { updateController.completeAuthorization(it.data) }

    override fun onCreate(savedInstanceState: android.os.Bundle?) {
        super.onCreate(savedInstanceState)
        dispatcher = NativeCommandDispatcher { command, _, _ ->
            when (command) {
                HostCommand.REQUEST_APPLICATION_UPDATE -> updateController.start()
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
        updateController = androidappkit.update.GoogleDriveUpdateController(
            this,
            androidappkit.update.GoogleDriveUpdateConfiguration(
                listOf("Android", "{{Application}}"),
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
            ),
            authorizeUpdate::launch,
            { message ->
                if (!isDestroyed) {
                    mainPage.setStatus(message)
                }
            },
            androidappkit.diagnostics.sharedLogger(),
            { onConfirmed, onCancelled ->
                confirmation = android.app.AlertDialog.Builder(this)
                    .setTitle("Reinstall application?")
                    .setMessage("This version is already installed.")
                    .setPositiveButton("Reinstall") { _, _ -> onConfirmed() }
                    .setNegativeButton("Cancel") { _, _ -> onCancelled() }
                    .setOnCancelListener { onCancelled() }
                    .show()
            },
        )
        setContentView(mainPage)
        onBackPressedDispatcher.addCallback(this, object : androidx.activity.OnBackPressedCallback(true) {
            override fun handleOnBackPressed() {
                mainPage.navigateBack { finish() }
            }
        })
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
        confirmation?.dismiss()
        mainPage.destroySession()
        super.onDestroy()
    }
}