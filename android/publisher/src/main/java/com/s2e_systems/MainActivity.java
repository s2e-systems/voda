package com.s2e_systems;

import android.app.Activity;
import android.content.SharedPreferences;
import android.util.Log;
import android.view.SurfaceHolder;
import android.widget.CompoundButton;
import android.widget.Toast;
import android.os.Bundle;

import androidx.annotation.NonNull;

import org.freedesktop.gstreamer.GStreamer;
import com.s2e_systems.databinding.ActivityMainBinding;



class SurfaceHolderCallback implements SurfaceHolder.Callback {
    private static native void nativeSurfaceInit(Object surface);
    private static native void nativeSurfaceFinalize(Object surface);

    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        nativeSurfaceInit(holder.getSurface());
    }

    public void surfaceCreated(SurfaceHolder holder) {
        nativeSurfaceInit(holder.getSurface());
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
       nativeSurfaceFinalize(holder.getSurface());
    }
}

public class MainActivity extends Activity implements CompoundButton.OnCheckedChangeListener {
    static {
        System.loadLibrary("voda");
    }
    private static native void nativeRunPublisher();
    private static native void nativeRunSubscriber();
    private ActivityMainBinding binding;

    private SharedPreferences mPrefs;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        try {
            GStreamer.init(this);
        } catch (Exception e) {
            Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
        }
        Log.i("VoDA","GStreamer initialized");

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
        binding.surfaceVideo.getHolder().addCallback(new SurfaceHolderCallback());

        mPrefs = getSharedPreferences(getLocalClassName(), MODE_PRIVATE);
        Boolean is_checked = mPrefs.getBoolean("is_checked", false);
        
        binding.toggleButton.setChecked(is_checked);
        onCheckedChanged(binding.toggleButton, is_checked);
        binding.toggleButton.setOnCheckedChangeListener(this);
    }

    protected void onPause() {
        super.onPause();

        SharedPreferences.Editor ed = mPrefs.edit();

        Log.i("VoDA", "onPause called" + binding.toggleButton.isChecked());
        ed.putBoolean("is_checked", binding.toggleButton.isChecked());
        ed.commit();
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        if (isChecked) {
            nativeRunSubscriber();
        } else {
            nativeRunPublisher();
        }
    }
}

