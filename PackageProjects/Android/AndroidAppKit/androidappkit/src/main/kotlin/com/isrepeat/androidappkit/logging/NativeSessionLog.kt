package com.isrepeat.androidappkit.logging

// Создаёт один журнал сеанса и передаёт file descriptor нативной части приложения.
class NativeSessionLog(
    private val identity: com.isrepeat.androidappkit.AppIdentity,
    private val configureNativeLog: com.isrepeat.androidappkit.NativeLogConfigurator,
) {
    private var uri: android.net.Uri? = null

    @Synchronized
    fun configure(context: android.content.Context): android.net.Uri {
        uri?.let { return it }
        val sessionLog = com.isrepeat.androidcoresdk.androidcoresdk.logging.MediaStoreSessionLog.create(
            context,
            identity.storageDirectory,
            identity.name,
        )
        configureNativeLog.configure(sessionLog.nativePath)
        return sessionLog.uri.also { uri = it }
    }

    @Synchronized
    fun currentUri(): android.net.Uri? = uri
}