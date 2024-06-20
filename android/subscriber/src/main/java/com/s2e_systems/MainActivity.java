package com.s2e_systems;

import android.app.Activity;
import android.util.Log;
import android.view.SurfaceHolder;
import android.widget.Toast;
import android.os.Bundle;
import android.content.res.Configuration;
import androidx.annotation.NonNull;
import android.system.Os;
import org.freedesktop.gstreamer.GStreamer;
import com.s2e_systems.databinding.ActivityMainBinding;

class SurfaceHolderCallback implements SurfaceHolder.Callback {
    private static native void nativeSurfaceInit(Object surface);
    private static native void nativeSurfaceFinalize(Object surface);

    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.d("MainGStreamer", "Surface changed to format " + format + " width "
                + width + " height " + height);
        nativeSurfaceInit(holder.getSurface());
    }

    public void surfaceCreated(@NonNull SurfaceHolder holder) {}

    public void surfaceDestroyed(SurfaceHolder holder) {
        nativeSurfaceFinalize(holder.getSurface());
    }
}

public class MainActivity extends Activity {
    static {
        System.loadLibrary("voda");
    }
    private static native void nativeRun();

    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        com.s2e_systems.databinding.ActivityMainBinding binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
        binding.surfaceVideo.getHolder().addCallback(new SurfaceHolderCallback());
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        try {
            Os.setenv("GST_DEBUG", "ahcsrc:3", true);
        } catch (Exception e) {
            Log.i("VoDA","Cannot set environment variables");
        }

        try {
            GStreamer.init(this);
        } catch (Exception e) {
            Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
        }
        Log.i("VoDA","GStreamer initialized");
        nativeRun();
        onConfigurationChanged(this.getResources().getConfiguration());
    }
}