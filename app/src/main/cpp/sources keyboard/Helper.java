package com.mxp;

import android.app.Activity;
import android.content.Context;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.widget.Toast;

import java.io.File;

public class Helper {

    public static Activity activity;
    private static KeyboardView keyboardView;

    public static void setActivity(Activity a) {
        activity = a;
        keyboardView = null;
    }

    public static void showToast(String msg) {
        if (activity == null)
            return;
        new Handler(Looper.getMainLooper()).post(
            () -> Toast.makeText(activity, msg, Toast.LENGTH_SHORT).show());
    }

    public static String getDocumentsPath() {
        File dir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS);
        return dir.getAbsolutePath();
    }

    public static String getDownloadPath() {
        File dir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
        return dir.getAbsolutePath();
    }

    public static String getPicturePath() {
        File dir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES);
        return dir.getAbsolutePath();
    }

    public static String getDCIMPath() {
        File dir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM);
        return dir.getAbsolutePath();
    }

    public static void showKeyboard(boolean show) {
        if (activity == null)
            return;

        new Handler(Looper.getMainLooper()).post(() -> {
            InputMethodManager imm = (InputMethodManager)
                activity.getSystemService(Context.INPUT_METHOD_SERVICE);

            if (imm == null)
                return;

            if (show) {
                if (keyboardView == null) {
                    keyboardView = new KeyboardView(activity);
                    activity.addContentView(keyboardView,
                        new android.view.ViewGroup.LayoutParams(1, 1));
                }

                final KeyboardView kv = keyboardView;
                kv.post(() -> {
                    kv.requestFocus();
                    imm.showSoftInput(kv, InputMethodManager.SHOW_FORCED);
                });
            } else {
                if (keyboardView != null) {
                    imm.hideSoftInputFromWindow(keyboardView.getWindowToken(), 0);
                    keyboardView.clearFocus();
                }
            }
        });
    }

    public static native void nativeSendChar(int unicode);
    public static native void nativeSendKey(int keyCode);

    static class KeyboardView extends View {

        public KeyboardView(Context ctx) {
            super(ctx);
            setFocusable(true);
            setFocusableInTouchMode(true);
        }

        @Override
        public boolean onCheckIsTextEditor() {
            return true;
        }

        @Override
        public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
            outAttrs.inputType = android.text.InputType.TYPE_CLASS_TEXT |
                                 android.text.InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
            outAttrs.imeOptions = EditorInfo.IME_ACTION_SEND |
                                  EditorInfo.IME_FLAG_NO_EXTRACT_UI |
                                  EditorInfo.IME_FLAG_NO_FULLSCREEN;
            outAttrs.initialSelStart = 0;
            outAttrs.initialSelEnd = 0;

            return new BaseInputConnection(this, false) {

                @Override
                public boolean commitText(CharSequence text, int newCursorPosition) {
                    for (int i = 0; i < text.length(); i++) {
                        char c = text.charAt(i);
                        if (Character.isHighSurrogate(c) && i + 1 < text.length()
                                && Character.isLowSurrogate(text.charAt(i + 1))) {
                            nativeSendChar(Character.toCodePoint(c, text.charAt(i + 1)));
                            i++;
                        } else {
                            nativeSendChar((int) c);
                        }
                    }
                    return true;
                }

                @Override
                public boolean deleteSurroundingText(int beforeLength, int afterLength) {
                    for (int i = 0; i < beforeLength; i++)
                        nativeSendKey(KeyEvent.KEYCODE_DEL);
                    for (int i = 0; i < afterLength; i++)
                        nativeSendKey(KeyEvent.KEYCODE_FORWARD_DEL);
                    return true;
                }

                @Override
                public boolean sendKeyEvent(KeyEvent event) {
                    if (event.getAction() == KeyEvent.ACTION_DOWN) {
                        int code = event.getKeyCode();
                        if (code == KeyEvent.KEYCODE_DEL ||
                            code == KeyEvent.KEYCODE_ENTER ||
                            code == KeyEvent.KEYCODE_DPAD_LEFT ||
                            code == KeyEvent.KEYCODE_DPAD_RIGHT) {
                            nativeSendKey(code);
                            return true;
                        }
                        int uni = event.getUnicodeChar(event.getMetaState());
                        if (uni != 0) {
                            nativeSendChar(uni);
                            return true;
                        }
                    }
                    return super.sendKeyEvent(event);
                }
            };
        }
    }
}