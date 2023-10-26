package com.s2e_systems;

import android.content.res.Configuration;
import android.os.Bundle;
import android.widget.TextView;
import android.app.Activity;
import android.util.Log;
import android.view.SurfaceHolder;

import androidx.annotation.NonNull;

import org.freedesktop.gstreamer.GStreamer;
import com.s2e_systems.subscriber.databinding.ActivityMainBinding;

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

    private native long nativeSubscriberInit(int orientation);
    private native void nativeSubscriberFinalize();

    private ActivityMainBinding binding;

    static {
        System.loadLibrary("gstreamer_android");
        System.loadLibrary("android_subscriber");
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        try {
            GStreamer.init(this);
        } catch (Exception e) {
            setMessage("GStreamer init failed");
        }

        onConfigurationChanged(this.getResources().getConfiguration());
    }

    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        long video_sink = nativeSubscriberInit(newConfig.orientation);
        if (video_sink == 0) {
            setMessage("Native subscriber init failed");
            return;
        }
        SurfaceHolderCallback surfaceHolderCallback = new SurfaceHolderCallback(video_sink);
        binding.surfaceVideo.getHolder().addCallback(surfaceHolderCallback);
    }

    protected void onDestroy() {
        nativeSubscriberFinalize();
        super.onDestroy();
    }

    // Called from native code. This sets the content of the TextView from the UI thread.
    private void setMessage(final String message) {
        final TextView tv = binding.textviewMessage;
        runOnUiThread(() -> tv.setText(message));
    }
}
