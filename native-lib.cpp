#include <Includes.hpp>

static char inputText[256] = "";
static char inputMulti[1024] = "";
static bool jetpack_hack = false;

void (*orig_Fly)(void* thiz, float targetHeight, float duration, float flightSpeed, float heightAdjustTime, int mode);
void my_Fly(void* thiz, float targetHeight, float duration, float flightSpeed, float heightAdjustTime, int mode) {
    if (jetpack_hack) {
        targetHeight = 25.0f;        // Terbang lebih tinggi
        duration = 99999.0f;         // Terbang selamanya
        flightSpeed = 70.0f;         // Kecepatan terbang lebih cepat
        mode = 1;                    // Paksa mode Jetpack (FlightMode.Jetpack = 1)
    }
    orig_Fly(thiz, targetHeight, duration, flightSpeed, heightAdjustTime, mode);
}

void DrawMenu() {

    ImGui::Begin("Demo");
    if (ImGui::BeginTabBar("MainTab")) {

        if (ImGui::BeginTabItem("Main")) {

            ImGui::Checkbox("Infinite Jetpack", &jetpack_hack);
            ImGui::Separator();
            ImGui::InputText("Input", inputText, sizeof(inputText));
            ImGui::InputTextMultiline("Multi", inputMulti, sizeof(inputMulti), ImVec2(-1, 100));

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Paths")) {

            static std::string documents = GetDocumentsPath();
            static std::string download = GetDownloadPath();
            static std::string pictures = GetPicturePath();
            static std::string dcim = GetDCIMPath();

            ImGui::Text("Documents: %s", documents.c_str());
            ImGui::Text("Download: %s", download.c_str());
            ImGui::Text("Pictures: %s", pictures.c_str());
            ImGui::Text("DCIM: %s", dcim.c_str());

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

EGLBoolean(*orig_eglSwapBuffers)(EGLDisplay, EGLSurface);
EGLBoolean _eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    eglQuerySurface(dpy, surface, EGL_WIDTH, &glWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &glHeight);

    if (!setup) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        auto& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)glWidth, (float)glHeight);
        io.IniFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.Fonts->AddFontFromFileTTF("/system/fonts/NotoSansCJK-Regular.ttc", 40.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());

        ImGui::StyleColorsDark();
        ImGui::GetStyle().ScaleAllSizes(2.5f);
        ImGui_ImplOpenGL3_Init("#version 300 es");
        setup = true;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame(glWidth, glHeight);
    static bool prevWantTextInput = false;
    bool wantTextInput = ImGui::GetIO().WantTextInput;
    if (wantTextInput != prevWantTextInput) {
        prevWantTextInput = wantTextInput;
        ShowKeyboard(wantTextInput);
    }
    DrainInputQueue();
    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2((float)glWidth / 2, (float)glHeight / 2), ImGuiCond_Once);
    DrawMenu();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return orig_eglSwapBuffers(dpy, surface);
}

void *input_thread(void *) {
    void* egl = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    void* inp = DobbySymbolResolver("libinput.so", "_ZN7android13InputConsumer21initializeMotionEventEPNS_11MotionEventEPKNS_12InputMessageE");

    if (egl) DobbyHook(egl, (void*)_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
    if (inp) DobbyHook(inp, (void*)myInput, (void**)&origInput);

    return nullptr;
}

void *MainThread(void *) {
    bool load = false;
    for (int i = 0; i < 10; i++) {
        void *handle = xdl_open("libil2cpp.so", 0);
        if (handle) {
            load = true;
            il2cpp_api_init(handle);
            il2cpp_dump();

            // Setup Hook
            auto charMotorClass = UnityResolve::Get("Assembly-CSharp.dll")
                ->Get("CharacterMotor", "SYBO.RunnerCore.Character");
            if (charMotorClass) {
                auto flyMethod = charMotorClass->Get<UnityResolve::Method>("Fly");
                if (flyMethod && flyMethod->function) {
                    DobbyHook(flyMethod->function, (void*)my_Fly, (void**)&orig_Fly);
                    LOGI("Fly Hooked Successfully!");
                } else {
                    LOGW("Failed to find Fly method!");
                }
            } else {
                LOGW("Failed to find CharacterMotor class!");
            }

            break;
        } else {
            sleep(1);
        }
    }
    if (!load) {
        LOGI("libil2cpp.so not found in thread %d", gettid());
    }
    return nullptr;
}

__attribute__((constructor))
void libmain() {
    pthread_t thread1, thread2;
    pthread_create(&thread1, nullptr, input_thread, nullptr);
    pthread_create(&thread2, nullptr, MainThread, nullptr);
    pthread_detach(thread1);
    pthread_detach(thread2);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    jvm = vm;
    // Load Dex From Memory
    LoadDex(imgui_dex, imgui_dex_len);
   // Toast("Testing Toast");
    return JNI_VERSION_1_6;
}
