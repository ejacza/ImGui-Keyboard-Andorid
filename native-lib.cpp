#include <Includes.hpp>

static char inputText[256] = "";
static char inputMulti[1024] = "";
static Gum::RefPtr<Gum::Interceptor> gumppLinkAnchor;
static ScanEngine::RegionType scannerRegion = ScanEngine::RegionType::ALL;
static ScanEngine::ValueType scannerType = ScanEngine::ValueType::DWORD;
static char scannerValue[96] = "";
static char scannerEditValue[96] = "";
static int scannerSelectedResult = -1;
static GumAddress scannerSelectedAddress = 0;
static bool scannerOpenEdit = false;

static void DrawScannerRegion()
{
    static constexpr ScanEngine::RegionType regions[] = {
        ScanEngine::RegionType::ALL,
        ScanEngine::RegionType::JAVA_HEAP,
        ScanEngine::RegionType::C_HEAP,
        ScanEngine::RegionType::C_ALLOC,
        ScanEngine::RegionType::C_DATA,
        ScanEngine::RegionType::C_BSS,
        ScanEngine::RegionType::PPSSPP,
        ScanEngine::RegionType::ANONYMOUS,
        ScanEngine::RegionType::JAVA,
        ScanEngine::RegionType::STACK,
        ScanEngine::RegionType::ASHMEM,
        ScanEngine::RegionType::VIDEO,
        ScanEngine::RegionType::OTHER,
        ScanEngine::RegionType::BAD,
        ScanEngine::RegionType::CODE_APP,
        ScanEngine::RegionType::CODE_SYS
    };
    if (ImGui::BeginCombo("Region", ScanEngine::RegionTypeName(scannerRegion)))
    {
        for (ScanEngine::RegionType region : regions)
        {
            bool selected = scannerRegion == region;
            if (ImGui::Selectable(ScanEngine::RegionTypeName(region), selected)) scannerRegion = region;
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

static void DrawScannerType()
{
    static constexpr ScanEngine::ValueType types[] = {
        ScanEngine::ValueType::DWORD,
        ScanEngine::ValueType::FLOAT,
        ScanEngine::ValueType::DOUBLE,
        ScanEngine::ValueType::WORD,
        ScanEngine::ValueType::BYTE,
        ScanEngine::ValueType::QWORD
    };
    if (ImGui::BeginCombo("Type", ScanEngine::ValueTypeName(scannerType)))
    {
        for (ScanEngine::ValueType type : types)
        {
            bool selected = scannerType == type;
            if (ImGui::Selectable(ScanEngine::ValueTypeName(type), selected)) scannerType = type;
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

static void DrawScannerResults()
{
    ScanEngine::Snapshot snapshot = ScanEngine::GetSnapshot();
    if (ImGui::BeginTable("ScanResults", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 420.0f)))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 230.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 130.0f);
        ImGui::TableHeadersRow();
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(snapshot.results.size()));
        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
            {
                ScanEngine::Result result = snapshot.results[static_cast<size_t>(row)];
                ScanEngine::RefreshResult(static_cast<size_t>(row), result);
                std::string value = ScanEngine::FormatValue(result.type, result.bytes.data());
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("0x%llX", static_cast<unsigned long long>(result.address));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(value.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::PushID(static_cast<int>(row));
                if (ImGui::Button("Edit"))
                {
                    scannerSelectedResult = row;
                    scannerSelectedAddress = result.address;
                    std::snprintf(scannerEditValue, sizeof(scannerEditValue), "%s", value.c_str());
                    scannerOpenEdit = true;
                }
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }
    if (scannerOpenEdit)
    {
        scannerOpenEdit = false;
        ImGui::OpenPopup("Edit Value");
    }
    if (ImGui::BeginPopupModal("Edit Value", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Address: 0x%llX", static_cast<unsigned long long>(scannerSelectedAddress));
        ImGui::InputText("New Value", scannerEditValue, sizeof(scannerEditValue));
        if (ImGui::Button("Write"))
        {
            ScanEngine::Result current;
            if (scannerSelectedResult >= 0 && ScanEngine::RefreshResult(static_cast<size_t>(scannerSelectedResult), current) && current.address == scannerSelectedAddress && ScanEngine::WriteResult(static_cast<size_t>(scannerSelectedResult), scannerEditValue)) ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

static void DrawScanner()
{
    DrawScannerRegion();
    DrawScannerType();
    ImGui::InputText("Value", scannerValue, sizeof(scannerValue));
    bool busy = ScanEngine::IsBusy();
    ImGui::BeginDisabled(busy);
    if (ImGui::Button("Search")) ScanEngine::Search(scannerType, scannerRegion, scannerValue);
    ImGui::SameLine();
    if (ImGui::Button("Refine")) ScanEngine::Refine(scannerType, scannerValue);
    ImGui::SameLine();
    if (ImGui::Button("Clear")) ScanEngine::Clear();
    ImGui::EndDisabled();
    if (busy)
    {
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) ScanEngine::Cancel();
    }
    std::string status = ScanEngine::GetStatus(ScanEngine::GetState());
    size_t count = ScanEngine::GetResultCount();
    ImGui::Text("Status: %s", status.c_str());
    ImGui::Text("Results: %zu", count);
    if (busy) ImGui::Text("Processed: %zu  Unreadable: %zu", ScanEngine::GetProcessed(), ScanEngine::GetUnreadable());
    if (count > ScanEngine::DisplayLimit) ImGui::Text("Showing first %zu results", ScanEngine::DisplayLimit);
    DrawScannerResults();
}

void DrawMenu() {

    ImGui::Begin("Demo");
    if (ImGui::BeginTabBar("MainTab")) {

        if (ImGui::BeginTabItem("Main")) {

            ImGui::InputText("Input", inputText, sizeof(inputText));
            ImGui::InputTextMultiline("Multi", inputMulti, sizeof(inputMulti), ImVec2(-1, 100));

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Scanner")) {

            DrawScanner();

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
    gum_init_embedded();
    gumppLinkAnchor = Gum::RefPtr<Gum::Interceptor>(Gum::Interceptor_obtain());
    pthread_t thread1, thread2;
    pthread_create(&thread1, nullptr, input_thread, nullptr);
    pthread_create(&thread2, nullptr, MainThread, nullptr);
    pthread_detach(thread1);
    pthread_detach(thread2);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    jvm = vm;
    LoadDex(imgui_dex, imgui_dex_len);
    return JNI_VERSION_1_6;
}
