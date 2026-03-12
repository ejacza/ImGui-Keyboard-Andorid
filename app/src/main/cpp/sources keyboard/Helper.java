package com.mxp;

import android.app.Activity;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.widget.Toast;

public class Helper {

    public static Activity activity;
    private static KeyboardView keyboardView;

    public static void setActivity(Activity a) {
        activity = a;
    }

    public static void showToast(String msg) {
        if (activity == null)
            return;
        new Handler(Looper.getMainLooper()).post(
            () -> Toast.makeText(activity, msg, Toast.LENGTH_SHORT).show());
    }

    public static void showKeyboard(boolean show) {
        if (activity == null)
            return;

        new Handler(Looper.getMainLooper()).post(() -> {
            InputMethodManager imm = (InputMethodManager)
                activity.getSystemService(Context.INPUT_METHOD_SERVICE);

            if (show) {
                if (keyboardView == null) {
                    keyboardView = new KeyboardView(activity);
                    android.view.ViewGroup.LayoutParams lp =
                        new android.view.ViewGroup.LayoutParams(1, 1);
                    activity.addContentView(keyboardView, lp);
                }
                keyboardView.requestFocus();
                imm.showSoftInput(keyboardView, InputMethodManager.SHOW_FORCED);
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
                        if (code == KeyEvent.KEYCODE_DEL) {
                            nativeSendKey(KeyEvent.KEYCODE_DEL);
                            return true;
                        } else if (code == KeyEvent.KEYCODE_ENTER) {
                            nativeSendKey(KeyEvent.KEYCODE_ENTER);
                            return true;
                        } else if (code == KeyEvent.KEYCODE_DPAD_LEFT) {
                            nativeSendKey(KeyEvent.KEYCODE_DPAD_LEFT);
                            return true;
                        } else if (code == KeyEvent.KEYCODE_DPAD_RIGHT) {
                            nativeSendKey(KeyEvent.KEYCODE_DPAD_RIGHT);
                            return true;
                        } else {
                            int uni = event.getUnicodeChar(event.getMetaState());
                            if (uni != 0) {
                                nativeSendChar(uni);
                                return true;
                            }
                        }
                    }
                    return super.sendKeyEvent(event);
                }
            };
        }
    }
}
