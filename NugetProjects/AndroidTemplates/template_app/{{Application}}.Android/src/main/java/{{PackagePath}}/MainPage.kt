package {{PackageId}}

import android.content.Context
import android.opengl.GLSurfaceView
import android.view.MotionEvent
import java.util.concurrent.CountDownLatch
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class MainPage(context: Context) : GLSurfaceView(context), GLSurfaceView.Renderer {
    private var handle = nativeCreate()

    init {
        setEGLContextClientVersion(3)
        setRenderer(this)
    }

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) = Unit

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        nativeSurface(handle, width, height)
    }

    override fun onDrawFrame(gl: GL10?) {
        nativeRender(handle)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
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
        val released = CountDownLatch(1)
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

    fun destroySession() {
        nativeDestroy(handle)
        handle = 0
    }

    private external fun nativeCreate(): Long
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