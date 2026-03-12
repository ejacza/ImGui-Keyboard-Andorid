#include <jni.h>
#include <dex_data.h>

inline JavaVM *jvm = nullptr;
inline bool Loader = false;
inline jobject gDexLoader = nullptr;

static JNIEnv *GetEnv(bool *attached)
{
    JNIEnv *env = nullptr;
    *attached = false;

    jint result = jvm->GetEnv((void **)&env, JNI_VERSION_1_6);

    if (result == JNI_EDETACHED) {
        jvm->AttachCurrentThread(&env, nullptr);
        *attached = true;
    }

    return env;
}

static jclass GetClass(JNIEnv *env, const char *className)
{
    if (!gDexLoader)
        return nullptr;

    jclass loaderClass = env->GetObjectClass(gDexLoader);
    jmethodID loadClass = env->GetMethodID(loaderClass, "loadClass",
                                           "(Ljava/lang/String;)Ljava/lang/Class;");
    env->DeleteLocalRef(loaderClass);

    if (!loadClass)
        return nullptr;

    jstring name = env->NewStringUTF(className);
    jclass result = (jclass)env->CallObjectMethod(gDexLoader, loadClass, name);
    env->DeleteLocalRef(name);

    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }

    return result;
}

static jobject GetActivity(JNIEnv *env)
{
    jclass activityThread = env->FindClass("android/app/ActivityThread");

    jmethodID currentActivityThread = env->GetStaticMethodID(
        activityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");

    jobject thread = env->CallStaticObjectMethod(activityThread, currentActivityThread);

    jfieldID activitiesField =
        env->GetFieldID(activityThread, "mActivities", "Landroid/util/ArrayMap;");

    jobject activities = env->GetObjectField(thread, activitiesField);

    jclass mapClass = env->FindClass("android/util/ArrayMap");

    jmethodID values = env->GetMethodID(mapClass, "values", "()Ljava/util/Collection;");

    jobject collection = env->CallObjectMethod(activities, values);

    jclass collectionClass = env->FindClass("java/util/Collection");

    jmethodID iterator =
        env->GetMethodID(collectionClass, "iterator", "()Ljava/util/Iterator;");

    jobject it = env->CallObjectMethod(collection, iterator);

    jclass iteratorClass = env->FindClass("java/util/Iterator");

    jmethodID hasNext = env->GetMethodID(iteratorClass, "hasNext", "()Z");
    jmethodID next = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");

    jobject activity = nullptr;

    if (env->CallBooleanMethod(it, hasNext)) {
        jobject record = env->CallObjectMethod(it, next);

        jclass recordClass = env->GetObjectClass(record);

        jfieldID activityField =
            env->GetFieldID(recordClass, "activity", "Landroid/app/Activity;");

        activity = env->GetObjectField(record, activityField);
    }

    return activity;
}

static void ActivitySet(JNIEnv *env)
{
    jobject activity = GetActivity(env);
    if (!activity)
        return;

    jclass helper = GetClass(env, "com.mxp.Helper");
    if (!helper)
        return;

    jmethodID setActivity =
        env->GetStaticMethodID(helper, "setActivity", "(Landroid/app/Activity;)V");

    if (!setActivity) {
        env->DeleteLocalRef(helper);
        return;
    }

    env->CallStaticVoidMethod(helper, setActivity, activity);
    env->DeleteLocalRef(helper);
}

static void nativeSendChar_impl(JNIEnv *, jclass, jint unicode)
{
    ImGuiIO &io = ImGui::GetIO();
    io.AddInputCharacter((unsigned int)unicode);
}

static void nativeSendKey_impl(JNIEnv *, jclass, jint keyCode)
{
    ImGuiIO &io = ImGui::GetIO();

    switch (keyCode) {
        case 67:
            io.AddKeyEvent(ImGuiKey_Backspace, true);
            io.AddKeyEvent(ImGuiKey_Backspace, false);
            break;
        case 112:
            io.AddKeyEvent(ImGuiKey_Delete, true);
            io.AddKeyEvent(ImGuiKey_Delete, false);
            break;
        case 66:
            io.AddKeyEvent(ImGuiKey_Enter, true);
            io.AddKeyEvent(ImGuiKey_Enter, false);
            break;
        case 21:
            io.AddKeyEvent(ImGuiKey_LeftArrow, true);
            io.AddKeyEvent(ImGuiKey_LeftArrow, false);
            break;
        case 22:
            io.AddKeyEvent(ImGuiKey_RightArrow, true);
            io.AddKeyEvent(ImGuiKey_RightArrow, false);
            break;
        case 19:
            io.AddKeyEvent(ImGuiKey_UpArrow, true);
            io.AddKeyEvent(ImGuiKey_UpArrow, false);
            break;
        case 20:
            io.AddKeyEvent(ImGuiKey_DownArrow, true);
            io.AddKeyEvent(ImGuiKey_DownArrow, false);
            break;
        default:
            break;
    }
}

static void RegisterNativeMethods(JNIEnv *env)
{
    jclass helper = GetClass(env, "com.mxp.Helper");
    if (!helper)
        return;

    JNINativeMethod methods[] = {
        { "nativeSendChar", "(I)V", (void *)nativeSendChar_impl },
        { "nativeSendKey",  "(I)V", (void *)nativeSendKey_impl  },
    };

    env->RegisterNatives(helper, methods, 2);
    env->DeleteLocalRef(helper);

    if (env->ExceptionCheck())
        env->ExceptionClear();
}

static void LoadDex(unsigned char *dex, int size)
{
    bool attached;
    JNIEnv *env = GetEnv(&attached);
    if (!env)
        return;

    jbyteArray dexArray = env->NewByteArray(size);
    env->SetByteArrayRegion(dexArray, 0, size, (jbyte *)dex);

    jclass bufferClass = env->FindClass("java/nio/ByteBuffer");

    jmethodID wrap =
        env->GetStaticMethodID(bufferClass, "wrap", "([B)Ljava/nio/ByteBuffer;");

    jobject buffer = env->CallStaticObjectMethod(bufferClass, wrap, dexArray);

    jclass loaderClass = env->FindClass("dalvik/system/InMemoryDexClassLoader");

    jclass contextClass = env->FindClass("android/app/ActivityThread");

    jmethodID currentApplication = env->GetStaticMethodID(
        contextClass, "currentApplication", "()Landroid/app/Application;");

    jobject app = env->CallStaticObjectMethod(contextClass, currentApplication);

    jclass ctx = env->FindClass("android/content/Context");

    jmethodID getClassLoader =
        env->GetMethodID(ctx, "getClassLoader", "()Ljava/lang/ClassLoader;");

    jobject parent = env->CallObjectMethod(app, getClassLoader);

    jmethodID ctor = env->GetMethodID(loaderClass, "<init>",
                                      "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");

    jobject dexLoader = env->NewObject(loaderClass, ctor, buffer, parent);

    if (dexLoader) {
        gDexLoader = env->NewGlobalRef(dexLoader);
        env->DeleteLocalRef(dexLoader);
    }

    jmethodID loadClass = env->GetMethodID(loaderClass, "loadClass",
                                           "(Ljava/lang/String;)Ljava/lang/Class;");

    jstring target = env->NewStringUTF("com.mxp.Helper");

    jobject clazz = env->CallObjectMethod(gDexLoader, loadClass, target);

    if (clazz) {
        Loader = true;
        env->DeleteLocalRef(clazz);
    }

    env->DeleteLocalRef(target);
    env->DeleteLocalRef(parent);
    env->DeleteLocalRef(buffer);
    env->DeleteLocalRef(dexArray);

    if (env->ExceptionCheck())
        env->ExceptionClear();

    ActivitySet(env);
    RegisterNativeMethods(env);

    if (attached)
        jvm->DetachCurrentThread();
}

static void Toast(const char *message)
{
    if (!Loader)
        return;

    bool attached;
    JNIEnv *env = GetEnv(&attached);
    if (!env)
        return;

    jclass helper = GetClass(env, "com.mxp.Helper");
    if (!helper) {
        if (attached)
            jvm->DetachCurrentThread();
        return;
    }

    jmethodID showToast =
        env->GetStaticMethodID(helper, "showToast", "(Ljava/lang/String;)V");

    if (showToast) {
        jstring msg = env->NewStringUTF(message);
        env->CallStaticVoidMethod(helper, showToast, msg);
        env->DeleteLocalRef(msg);
    }

    env->DeleteLocalRef(helper);

    if (attached)
        jvm->DetachCurrentThread();
}

static void ShowKeyboard(bool show)
{
    if (!Loader)
        return;

    bool attached;
    JNIEnv *env = GetEnv(&attached);
    if (!env)
        return;

    jclass helper = GetClass(env, "com.mxp.Helper");
    if (!helper) {
        if (attached)
            jvm->DetachCurrentThread();
        return;
    }

    jmethodID showKeyboard =
        env->GetStaticMethodID(helper, "showKeyboard", "(Z)V");

    if (showKeyboard)
        env->CallStaticVoidMethod(helper, showKeyboard, (jboolean)show);

    env->DeleteLocalRef(helper);

    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (attached)
        jvm->DetachCurrentThread();
}