package {{PackageId}}

class MainPage(context: android.content.Context, dispatcher: NativeCommandDispatcher) : android.opengl.GLSurfaceView(context), android.opengl.GLSurfaceView.Renderer {
    private var handle = nativeCreate(dispatcher)
    private var destroyed = false

    init {
        setEGLContextClientVersion(3)
        setRenderer(this)
    }

    override fun onSurfaceCreated(gl: javax.microedition.khronos.opengles.GL10?, config: javax.microedition.khronos.egl.EGLConfig?) = Unit

    override fun onSurfaceChanged(gl: javax.microedition.khronos.opengles.GL10?, width: Int, height: Int) {
        nativeSurface(handle, width, height)
    }

    override fun onDrawFrame(gl: javax.microedition.khronos.opengles.GL10?) {
        nativeRender(handle)
    }

    override fun onTouchEvent(event: android.view.MotionEvent): Boolean {
        val action = event.actionMasked
        val x = event.x
        val y = event.y
        queueEvent { nativePointer(handle, action, x, y) }
        return true
    }

    fun navigateBack(onRoot: () -> Unit) {
        queueEvent {
            if (!nativeBack(handle)) {
                post { onRoot() }
            }
        }
    }

    fun pauseSession() {
        val released = java.util.concurrent.CountDownLatch(1)
        queueEvent {
            try {
                nativeReleaseSurface(handle)
            } finally {
                released.countDown()
            }
        }
        released.await()
        onPause()
    }

    fun setStatus(value: String) {
        if (destroyed) {
            return
        }
        val bytes = value.toByteArray(Charsets.UTF_8)
        queueEvent {
            if (handle != 0L) {
                nativeSetStatus(handle, bytes)
            }
        }
    }

    fun configureNativeLog(path: String) {
        if (!destroyed) {
            nativeConfigureLogFile(handle, path)
        }
    }

    fun destroySession() {
        if (destroyed) {
            return
        }
        destroyed = true
        // Барьер выполняет ранее поставленные события до удаления сессии.
        val released = java.util.concurrent.CountDownLatch(1)
        queueEvent {
            try {
                nativeDestroy(handle)
                handle = 0
            } finally {
                released.countDown()
            }
        }
        released.await()
    }

    private external fun nativeCreate(dispatcher: NativeCommandDispatcher): Long
    private external fun nativeConfigureLogFile(handle: Long, path: String)
    private external fun nativeSetStatus(handle: Long, value: ByteArray)
    private external fun nativeDestroy(handle: Long)
    private external fun nativeSurface(handle: Long, width: Int, height: Int)
    private external fun nativeReleaseSurface(handle: Long)
    private external fun nativeRender(handle: Long)
    private external fun nativePointer(handle: Long, action: Int, x: Float, y: Float)
    private external fun nativeBack(handle: Long): Boolean

    companion object {
        init {
            System.loadLibrary("{{application}}")
        }
    }
}