package com.s2e_systems;

import android.os.Bundle;
import android.widget.TextView;
import android.app.Activity;
import android.util.Log;
import android.view.SurfaceHolder;

import org.freedesktop.gstreamer.GStreamer;

import com.s2e_systems.publisher.databinding.ActivityMainBinding;



class SurfaceHolderCallback implements SurfaceHolder.Callback {
    final long video_sink;

    SurfaceHolderCallback(long video_sink) {
        this.video_sink = video_sink;
    }
    private native void nativeSurfaceInit(Object surface, long video_sink);
    private native void nativeSurfaceFinalize(Object surface);

    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.d("MainGStreamer", "Surface changed to format " + format + " width "
                + width + " height " + height);
        nativeSurfaceInit(holder.getSurface(), this.video_sink);
    }

    public void surfaceCreated(SurfaceHolder holder) {}

    public void surfaceDestroyed(SurfaceHolder holder) {
        nativeSurfaceFinalize(holder.getSurface());
    }
}

public class MainActivity extends Activity {

    private native long nativePublisherInit();
    private native void nativePublisherFinalize();

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
            Log.e("Publisher", "GStreamer.init failed");
        }

        SurfaceHolderCallback surfaceHolderCallback = new SurfaceHolderCallback(nativePublisherInit());

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        binding.surfaceVideo.getHolder().addCallback(surfaceHolderCallback);
        setContentView(binding.getRoot());
        binding.textviewMessage.setText("Starting ... ");
    }

    protected void onDestroy() {
        nativePublisherFinalize();
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
