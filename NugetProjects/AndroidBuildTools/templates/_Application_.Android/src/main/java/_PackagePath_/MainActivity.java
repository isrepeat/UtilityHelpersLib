package <PackageId>;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends Activity {
    static {
        System.loadLibrary("<application>");
    }

    private native String nativeMessage();

    @Override
    public void onCreate(Bundle state) {
        super.onCreate(state);
        TextView text = new TextView(this);
        text.setText(nativeMessage());
        setContentView(text);
    }
}