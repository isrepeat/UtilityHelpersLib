package com.isrepeat.androidappkit.update

data class GoogleDriveUpdateConfiguration(
    val driveFolderPath: List<String>,
    val apkNamePattern: Regex,
    val versionCodeFromName: (MatchResult) -> Long,
    val updaterPackage: String,
    val updaterActivity: String,
    val updaterPermission: String,
    val updaterAction: String,
    val targetPackageExtra: String = "target_package",
)

fun interface UpdateLogger {
    fun log(message: String)
}