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
     private native void nativeLibInit();
     private native void nativeFinalize();
     private native void nativeSurfaceInit(Object surface);
     private native void nativeSurfaceFinalize();
     private long native_custom_data;      // Native code will use this to keep private data

    private ActivityMainBinding binding;

    static {
         System.loadLibrary("gstreamer_android");
         System.loadLibrary("android_publisher");
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        binding.textviewMessage.setText("Starting ... ");

         try {
             GStreamer.init(this);
         } catch (Exception e) {
             binding.textviewMessage.setText("Failed to intialize GStreamer:" + e.getMessage());
         }
         binding.surfaceVideo.getHolder().addCallback(this);

        nativeLibInit();
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
         nativeSurfaceFinalize();
    }

}
