package mainactivity;

import android.os.Bundle;
import android.os.Handler;
import android.widget.TextView;

import mainactivity.databinding.ActivityMainBinding;

import android.app.Activity;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;


import org.freedesktop.gstreamer.GStreamer;

class SurfaceHolderCallback implements SurfaceHolder.Callback {
    private native void nativeSurfaceInit(Object surface, long video_sink);
    private native void nativeSurfaceFinalize(Object surface);

    SurfaceHolderCallback(long video_sink) {
        this.video_sink = video_sink;
    }
    long video_sink;

    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.d("MainGStreamer", "Surface changed to format " + format + " width "
                + width + " height " + height);
        nativeSurfaceInit(holder.getSurface(), this.video_sink);
    }

    public void surfaceCreated(SurfaceHolder holder) {
        Log.d("MainGStreamer", "Surface created: " + holder.getSurface());
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d("MainGStreamer", "Surface destroyed");
        nativeSurfaceFinalize(holder.getSurface());
    }
}

public class MainActivity extends Activity {

    private SurfaceHolderCallback surfaceHolderCallback;
    private native void nativeCustomDataInit();
    private native long nativeLibInit();
    private native void nativeFinalize(Object app, long gst_app_thread);
    private long native_custom_data;      // Native code will use this to keep private data
    private Object app;
    private long gst_app_thread;

    private ActivityMainBinding binding;

    static {
         System.loadLibrary("gstreamer_android");
         System.loadLibrary("android_publisher");
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        try {
            GStreamer.init(this);
        } catch (Exception e) {
            Log.e("MyGStreamer", "GStreamer.init failed");
        }

        nativeCustomDataInit();

        long native_video_overlay_pointer = nativeLibInit();
        Log.d("MyGStreamer", "nativeLibInit: " + Long.toHexString(native_video_overlay_pointer));
        surfaceHolderCallback = new SurfaceHolderCallback(native_video_overlay_pointer);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        binding.surfaceVideo.getHolder().addCallback(surfaceHolderCallback);
        setContentView(binding.getRoot());
        binding.textviewMessage.setText("Starting ... ");

//        try {
//            GStreamer.init(this);
//        } catch (Exception e) {
//            binding.textviewMessage.setText("Failed to intialize GStreamer:" + e.getMessage());
//        }
    }

    protected void onDestroy() {
        nativeFinalize(this.app, this.gst_app_thread);
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
}
