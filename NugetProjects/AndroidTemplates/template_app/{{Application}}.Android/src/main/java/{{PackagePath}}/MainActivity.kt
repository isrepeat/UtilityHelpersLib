package {{PackageId}}

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.OnBackPressedCallback

class MainActivity : ComponentActivity() {
    private lateinit var mainPage: MainPage

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        mainPage = MainPage(this)
        setContentView(mainPage)
        onBackPressedDispatcher.addCallback(this, object : OnBackPressedCallback(true) {
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
        mainPage.destroySession()
        super.onDestroy()
    }
}