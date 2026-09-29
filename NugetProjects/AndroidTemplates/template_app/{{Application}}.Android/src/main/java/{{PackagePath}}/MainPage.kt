package {{PackageId}}

import android.content.Context
import android.graphics.Color
import android.view.Gravity
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView

class MainPage(context: Context, checkForUpdate: () -> Unit) : LinearLayout(context) {
    private val status = TextView(context)

    init {
        orientation = VERTICAL
        gravity = Gravity.CENTER
        setPadding(48, 48, 48, 48)

        addView(TextView(context).apply {
            text = "{{Application}}"
            textSize = 24f
            setTextColor(Color.BLACK)
        })
        addView(Button(context).apply {
            text = "Update"
            setOnClickListener { checkForUpdate() }
        })
        addView(status.apply {
            text = "Ready to check Google Drive for an update."
            gravity = Gravity.CENTER
        })
    }

    fun setStatus(message: String) {
        status.text = message
    }
}