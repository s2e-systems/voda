package com.s2e_systems;

import android.app.Activity;
import android.util.Log;
import android.view.SurfaceHolder;
import android.widget.Toast;
import android.os.Bundle;
import androidx.annotation.NonNull;
import org.freedesktop.gstreamer.GStreamer;
import com.s2e_systems.databinding.ActivityMainBinding;

class SurfaceHolderCallback implements SurfaceHolder.Callback {
    private static native void nativeSurfaceInit(Object surface);
    private static native void nativeSurfaceFinalize(Object surface);

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
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
    private static native void nativeRunPublisher();
    private static native void nativeRunSubscriber();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        try {
            GStreamer.init(this);
        } catch (Exception e) {
            Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
        }
        Log.i("VoDA","GStreamer initialized");

//        nativeRunPublisher();
        nativeRunSubscriber();

        ActivityMainBinding binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
        binding.surfaceVideo.getHolder().addCallback(new SurfaceHolderCallback());
        binding.toggleButton.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                nativeRunSubscriber();
            } else {
                nativeRunPublisher();
            }
        });
    }
}