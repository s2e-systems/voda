package mainactivity;

import android.os.Bundle;
import android.widget.TextView;

import mainactivity.databinding.ActivityMainBinding;

import android.app.Activity;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;


import org.freedesktop.gstreamer.GStreamer;


public class MainActivity extends Activity implements SurfaceHolder.Callback {
     private native void nativeInit();
     private native void nativeFinalize();
     private native void nativePlay();
     private static native boolean nativeClassInit();
     private native void nativeSurfaceInit(Object surface);
     private native void nativeSurfaceFinalize();
     private long native_custom_data;      // Native code will use this to keep private data

    private ActivityMainBinding binding;

    static {
         System.loadLibrary("gstreamer_android");
         System.loadLibrary("android_publisher");
         nativeClassInit();
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        binding.textviewMessage.setText("starting ... ");

         try {
             GStreamer.init(this);
             SurfaceView sv = binding.surfaceVideo;
             SurfaceHolder sh = sv.getHolder();
             sh.addCallback(this);

             nativeInit();

         } catch (Exception e) {
             binding.textviewMessage.setText("Failed intializing gstreamer:" + e.getMessage());
         }
     }

     protected void onDestroy() {
         nativeFinalize();
         super.onDestroy();
     }

     // Called from native code. This sets the content of the TextView from the UI thread.
     private void setMessage(final String message) {
         final TextView tv = binding.textviewMessage;
         runOnUiThread (new Runnable() {
             public void run() {
                 tv.setText(message);
             }
         });
     }

     // Called from native code. Native code calls this once it has created its pipeline and
     // the main loop is running, so it is ready to accept commands.
     private void onGStreamerInitialized () {
         final TextView tv = binding.textviewMessage;
         tv.setText("GStreamer initialized");
         Log.i ("GStreamer", "Gst initialized. Start playing");

         nativePlay();
    }



    public void surfaceChanged(SurfaceHolder holder, int format, int width,
                               int height) {
         Log.d("GStreamer", "Surface changed to format " + format + " width "
                 + width + " height " + height);
         nativeSurfaceInit (holder.getSurface());
    }

    public void surfaceCreated(SurfaceHolder holder) {
         Log.d("GStreamer", "Surface created: " + holder.getSurface());
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
         Log.d("GStreamer", "Surface destroyed");
         nativeSurfaceFinalize ();
    }

}
