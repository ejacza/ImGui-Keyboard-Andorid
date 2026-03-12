# ImGui Android — Soft Keyboard via InMemoryDex

Dear ImGui overlay for Android with soft keyboard support. The keyboard bridge is a DEX payload embedded as a C byte array inside the `.so` — loaded at runtime via `InMemoryDexClassLoader`, no file written to disk.

---

## How It Works

ImGui has no Android `View` hierarchy, so the system doesn't know when to show the keyboard. This is solved by embedding `Helper.java` as a compiled DEX array (`dex_data.h`) and loading it with `InMemoryDexClassLoader` (API 26+) on `JNI_OnLoad`. The loaded class creates an invisible `View` that the IME attaches to, and bridges keyboard input back into `ImGuiIO` via JNI native methods.

```
JNI_OnLoad
    └── LoadDex(imgui_dex, imgui_dex_len)
            └── InMemoryDexClassLoader
                    └── com.mxp.Helper
                            ├── setActivity()       → bind current Activity
                            ├── showKeyboard(bool)  → show/hide soft keyboard
                            ├── nativeSendChar(int) → io.AddInputCharacter()
                            └── nativeSendKey(int)  → io.AddKeyEvent()
```

Every frame, `eglSwapBuffers` (hooked via Dobby) checks `ImGui::GetIO().WantTextInput` and calls `ShowKeyboard()` accordingly — automatically triggered when any `InputText` widget is focused.

---

## Key Files

| File | Description |
|---|---|
| `native-lib.cpp` | Entry point, render hook, input hook |
| `Includes/jni.hpp` | `LoadDex`, `GetClass`, `RegisterNativeMethods`, `ShowKeyboard`, `Toast` |
| `Includes/dex_data.h` | Compiled `Helper.java` as `unsigned char[]` |
| `sources keyboard/Helper.java` | Source of the DEX — reference only, not shipped |

---

## Regenerate `dex_data.h`

If you modify `Helper.java`:

```bash
javac -source 8 -target 8 Helper.java
d8 --min-api 26 Helper.class --output ./
xxd -i classes.dex > Includes/dex_data.h
```

---

## Build

```bash
# CMake
# configured in CMakeLists.txt — targets arm64-v8a, armeabi-v7a, C++20

# or NDK
ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk
```

Output: `libDemo.so`

---

## Injection

Open `AndroidManifest.xml` and find the `<activity>` that has:

```xml
<action android:name="android.intent.action.MAIN" />
<category android:name="android.intent.category.LAUNCHER" />
```

The `android:name` attribute on that `<activity>` is the launcher class — e.g. `com.appsomniacs.da2.DA2Activity`. Open its smali at `smali/com/appsomniacs/da2/DA2Activity.smali` and insert inside `onCreate`

```smali
const-string v0, "Demo"

invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
```

Place `libDemo.so` in `lib/arm64-v8a/` and/or `lib/armeabi-v7a/` inside the APK, then repack, sign, and install. `JNI_OnLoad` fires automatically on load.

---

## Requirements

- Android API 26+
- NDK r25+
- C++20

---

## Notes

- `Toast()` is available but commented out by default — uncomment in `JNI_OnLoad` to verify DEX loaded successfully.
- Font path `/system/fonts/NotoSansCJK-Regular.ttc` may not exist on all devices — adjust as needed.
- `ScaleAllSizes(2.5f)` is tuned for high-DPI mobile screens — adjust for your target resolution.
