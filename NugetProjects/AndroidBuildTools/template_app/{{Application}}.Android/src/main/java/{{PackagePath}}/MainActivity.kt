package {{PackageId}}

import android.app.AlertDialog
import android.content.Intent
import android.os.Bundle

import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts

import com.isrepeat.androidappkit.androidappkit

class MainActivity : ComponentActivity() {
    private lateinit var mainPage: MainPage
    private lateinit var updateController: androidappkit.update.GoogleDriveUpdateController

    private val authorizeGoogleDriveUpdate = registerForActivityResult(
        ActivityResultContracts.StartIntentSenderForResult(),
    ) { updateController.completeAuthorization(it.data) }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        mainPage = MainPage(this, ::checkForUpdate)
        updateController = androidappkit.update.GoogleDriveUpdateController(
            this,
            androidappkit.update.GoogleDriveUpdateConfiguration(
                listOf("Android", "{{Application}}"),
                Regex("{{Application}}-(\\d+)\\.(\\d+)\\.(\\d+)\\.apk", RegexOption.IGNORE_CASE),
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
            authorizeGoogleDriveUpdate::launch,
            mainPage::setStatus,
            androidappkit.update.UpdateLogger { },
            ::confirmSameVersion,
        )
        setContentView(mainPage)
    }

    private fun checkForUpdate() = updateController.start()

    private fun confirmSameVersion(onConfirmed: () -> Unit, onCancelled: () -> Unit) {
        AlertDialog.Builder(this)
            .setTitle("Reinstall the same version?")
            .setMessage("The version on Google Drive matches the installed version. Continue installation?")
            .setNegativeButton("Cancel") { _, _ -> onCancelled() }
            .setOnCancelListener { onCancelled() }
            .setPositiveButton("Install") { _, _ -> onConfirmed() }
            .show()
    }
}