package mainactivity;

import android.os.Bundle;
import android.widget.TextView;
import android.widget.Toast;

import mainactivity.databinding.ActivityMainBinding;

import android.app.Activity;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;


// import gstreamer.GStreamer;


public class MainActivity extends Activity implements SurfaceHolder.Callback {
    private native String nativeDdsInit();
    // private native void nativeInit();
    // private native void nativeFinalize(); // Destroy pipeline and shutdown native code
    // private native void nativePlay();     // Set pipeline to PLAYING
    // private native void nativePause();    // Set pipeline to PAUSED
    // private static native boolean nativeClassInit(); // Initialize native class: cache Method IDs for callbacks
    // private native void nativeSurfaceInit(Object surface);
    // private native void nativeSurfaceFinalize();
    // private long native_custom_data;      // Native code will use this to keep private data

    // private boolean is_playing_desired;   // Whether the user asked to go to PLAYING

    private ActivityMainBinding binding;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

    //     try {
    //         GStreamer.init(this);
    //     } catch (Exception e) {
    //         Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
    //         finish();
    //         return;
    //     }

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        final TextView tv = binding.textviewMessage;
        tv.setText("starting ... ");

        String ret = nativeDdsInit();
        tv.setText(ret);
//        Toast.makeText(this, ret, Toast.LENGTH_LONG).show();

    //     ImageButton play = binding.buttonPlay;
    //     play.setOnClickListener(new OnClickListener() {
    //         public void onClick(View v) {
    //             is_playing_desired = true;
    //             nativePlay();
    //         }
    //     });

    //     ImageButton pause = binding.buttonStop;
    //     pause.setOnClickListener(new OnClickListener() {
    //         public void onClick(View v) {
    //             is_playing_desired = false;
    //             nativePause();
    //         }
    //     });

    //     SurfaceView sv = binding.surfaceVideo;
    //     SurfaceHolder sh = sv.getHolder();
    //     sh.addCallback(this);

    //     if (savedInstanceState != null) {
    //         is_playing_desired = savedInstanceState.getBoolean("playing");
    //         Log.i ("GStreamer", "Activity created. Saved state is playing:" + is_playing_desired);
    //     } else {
    //         is_playing_desired = false;
    //         Log.i ("GStreamer", "Activity created. There is no saved state, playing: false");
    //     }

    //     // Start with disabled buttons, until native code is initialized
    //     binding.buttonPlay.setEnabled(false);
    //     binding.buttonStop.setEnabled(false);

//         nativeInit();
    // }

    // protected void onSaveInstanceState (Bundle outState) {
    //     Log.d ("GStreamer", "Saving state, playing:" + is_playing_desired);
    //     outState.putBoolean("playing", is_playing_desired);
    // }

    // protected void onDestroy() {
    //     nativeFinalize();
    //     super.onDestroy();
    // }

    // // Called from native code. This sets the content of the TextView from the UI thread.
    // private void setMessage(final String message) {
    //     final TextView tv = binding.textviewMessage;
    //     runOnUiThread (new Runnable() {
    //         public void run() {
    //             tv.setText(message);
    //         }
    //     });
    // }

    // // Called from native code. Native code calls this once it has created its pipeline and
    // // the main loop is running, so it is ready to accept commands.
    // private void onGStreamerInitialized () {
    //     Log.i ("GStreamer", "Gst initialized. Restoring state, playing:" + is_playing_desired);
    //     // Restore previous playing state
    //     if (is_playing_desired) {
    //         nativePlay();
    //     } else {
    //         nativePause();
    //     }

    //     // Re-enable buttons, now that GStreamer is initialized
    //     final Activity activity = this;
    //     runOnUiThread(new Runnable() {
    //         public void run() {
    //             binding.buttonPlay.setEnabled(true);
    //             binding.buttonStop.setEnabled(true);
    //         }
    //     });
    }

     static {
//         System.loadLibrary("gstreamer_android");
         System.loadLibrary("android_publisher");
//         nativeClassInit();
     }

    public void surfaceChanged(SurfaceHolder holder, int format, int width,
                               int height) {
        // Log.d("GStreamer", "Surface changed to format " + format + " width "
        //         + width + " height " + height);
        // nativeSurfaceInit (holder.getSurface());
    }

    public void surfaceCreated(SurfaceHolder holder) {
    //     Log.d("GStreamer", "Surface created: " + holder.getSurface());
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        // Log.d("GStreamer", "Surface destroyed");
        // nativeSurfaceFinalize ();
    }

}
