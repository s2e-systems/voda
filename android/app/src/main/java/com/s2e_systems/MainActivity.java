package com.s2e_systems;

import androidx.appcompat.app.AppCompatActivity;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.view.SurfaceHolder;
import android.widget.CompoundButton;
import android.widget.Toast;
import android.os.Bundle;
import androidx.annotation.NonNull;
import android.Manifest;
import android.content.pm.PackageManager;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import org.freedesktop.gstreamer.GStreamer;
import com.s2e_systems.databinding.ActivityMainBinding;

class SurfaceHolderCallback implements SurfaceHolder.Callback {
    private static native void nativeSurfaceInit(Object surface);
    private static native void nativeSurfaceFinalize(Object surface);

    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        nativeSurfaceInit(holder.getSurface());
    }

    public void surfaceCreated(@NonNull SurfaceHolder holder) {}

    public void surfaceDestroyed(SurfaceHolder holder) {
       nativeSurfaceFinalize(holder.getSurface());
    }
}

public class MainActivity extends AppCompatActivity implements CompoundButton.OnCheckedChangeListener {
    static {
        System.loadLibrary("voda");
    }
    private static native void nativeRunPublisher();
    private static native void nativeRunSubscriber();
    private static native void nativeRotationChanged(int rotation);
    private ActivityMainBinding binding;
    private SharedPreferences preferences;
    private static final int CAMERA_PERMISSION_CODE = 100;

    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        nativeRotationChanged(getWindowManager().getDefaultDisplay().getRotation());
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            GStreamer.init(this);
        } catch (Exception e) {
            Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
        }

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
        binding.surfaceVideo.getHolder().addCallback(new SurfaceHolderCallback());

        preferences = getSharedPreferences(getLocalClassName(), MODE_PRIVATE);
        boolean is_checked = preferences.getBoolean("isChecked", false);

        binding.toggleButton.setChecked(is_checked);

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            requestCameraPermission();
        } else {
            onCheckedChanged(binding.toggleButton, is_checked);
        }
        binding.toggleButton.setOnCheckedChangeListener(this);
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        if (isChecked) {
            nativeRunSubscriber();
        } else {
            nativeRunPublisher();
        }
        onConfigurationChanged(this.getResources().getConfiguration());
    }

    @Override
    protected void onPause() {
        super.onPause();
        SharedPreferences.Editor ed = preferences.edit();
        ed.putBoolean("isChecked", binding.toggleButton.isChecked());
        ed.apply();
    }

    private void requestCameraPermission() {
        binding.textviewMessage.setText(R.string.camera_needed);
        ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, CAMERA_PERMISSION_CODE);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == CAMERA_PERMISSION_CODE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                binding.textviewMessage.setText("");
                boolean is_checked = preferences.getBoolean("isChecked", false);
                onCheckedChanged(binding.toggleButton, is_checked);
            } else {
                binding.textviewMessage.setText(R.string.camera_permission_denied);
            }
        }
    }
}

