package com.raport.redalert;

import android.content.res.AssetManager;
import android.os.Build;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import org.libsdl.app.SDLActivity;

public class RedAlertActivity extends SDLActivity {
    private static final String TAG = "RedAlertActivity";
    private static final String ASSET_ROOT = "redalert";

    @Override
    protected String[] getLibraries() {
        return new String[] {"SDL2", "main"};
    }

    @Override
    protected String getMainFunction() {
        return "SDL_main";
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        configureImmersiveMode();
        extractBundledAssets();
        super.onCreate(savedInstanceState);
        configureImmersiveMode();
        installOverlayControls();
    }

    @Override
    protected void onResume() {
        super.onResume();
        configureImmersiveMode();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            configureImmersiveMode();
        }
    }

    private void configureImmersiveMode() {
        Window window = getWindow();
        window.addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            WindowManager.LayoutParams attributes = window.getAttributes();
            attributes.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            window.setAttributes(attributes);
        }

        View decorView = window.getDecorView();
        decorView.setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        );

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            window.setDecorFitsSystemWindows(false);
            WindowInsetsController controller = decorView.getWindowInsetsController();
            if (controller != null) {
                controller.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
                controller.hide(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
            }
        }
    }

    private void installOverlayControls() {
        LinearLayout controls = new LinearLayout(this);
        controls.setOrientation(LinearLayout.VERTICAL);
        controls.setAlpha(0.66f);
        controls.setPadding(dp(3), dp(3), dp(3), dp(3));

        addKeyButton(controls, "Esc", KeyEvent.KEYCODE_ESCAPE);
        addKeyButton(controls, "Sel", KeyEvent.KEYCODE_E);
        addKeyButton(controls, "Fix", KeyEvent.KEYCODE_T);
        addKeyButton(controls, "Sell", KeyEvent.KEYCODE_Y);
        addKeyButton(controls, "Map", KeyEvent.KEYCODE_U);

        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.WRAP_CONTENT,
            FrameLayout.LayoutParams.WRAP_CONTENT,
            Gravity.START | Gravity.CENTER_VERTICAL
        );
        params.leftMargin = dp(2);
        addContentView(controls, params);
    }

    private void addKeyButton(LinearLayout controls, String label, final int keyCode) {
        Button button = new Button(this);
        button.setText(label);
        button.setTextSize(10.0f);
        button.setAllCaps(false);
        button.setMinWidth(dp(34));
        button.setMinHeight(dp(30));
        button.setPadding(dp(3), 0, dp(3), 0);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                sendKeyPulse(keyCode);
            }
        });
        controls.addView(button);
    }

    private void sendKeyPulse(int keyCode) {
        long now = SystemClock.uptimeMillis();
        dispatchKeyEvent(new KeyEvent(now, now, KeyEvent.ACTION_DOWN, keyCode, 0));
        dispatchKeyEvent(new KeyEvent(now, now, KeyEvent.ACTION_UP, keyCode, 0));
    }

    private int dp(int value) {
        return (int)(value * getResources().getDisplayMetrics().density + 0.5f);
    }

    private void extractBundledAssets() {
        File resourceRoot = new File(getFilesDir(), "redalert-root");
        File config = new File(resourceRoot, "assets/redalert/allies/INSTALL/REDALERT.INI");
        if (config.isFile()) {
            return;
        }

        File target = new File(resourceRoot, "assets/redalert");
        deleteTree(target);
        try {
            copyAssetTree(getAssets(), ASSET_ROOT, target);
        } catch (IOException exception) {
            throw new IllegalStateException("Unable to extract bundled Red Alert assets", exception);
        }

        if (!config.isFile()) {
            throw new IllegalStateException("Bundled Red Alert assets are missing INSTALL/REDALERT.INI");
        }
        Log.i(TAG, "Extracted bundled Red Alert assets to " + target);
    }

    private static void copyAssetTree(AssetManager assets, String assetPath, File target) throws IOException {
        String[] children = assets.list(assetPath);
        if (children != null && children.length > 0) {
            if (!target.isDirectory() && !target.mkdirs()) {
                throw new IOException("Unable to create directory " + target);
            }
            for (String child : children) {
                copyAssetTree(assets, assetPath + "/" + child, new File(target, child));
            }
            return;
        }

        File parent = target.getParentFile();
        if (parent != null && !parent.isDirectory() && !parent.mkdirs()) {
            throw new IOException("Unable to create directory " + parent);
        }

        byte[] buffer = new byte[1024 * 64];
        try (InputStream input = assets.open(assetPath); OutputStream output = new FileOutputStream(target)) {
            int read;
            while ((read = input.read(buffer)) != -1) {
                output.write(buffer, 0, read);
            }
        }
    }

    private static void deleteTree(File path) {
        if (!path.exists()) {
            return;
        }
        File[] children = path.listFiles();
        if (children != null) {
            for (File child : children) {
                deleteTree(child);
            }
        }
        if (!path.delete()) {
            Log.w(TAG, "Unable to delete " + path);
        }
    }
}
