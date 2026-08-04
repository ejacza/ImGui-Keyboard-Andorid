cmake_minimum_required(VERSION 3.22.1)

project(Demo)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -flto -fvisibility=hidden -ffunction-sections -fdata-sections -fomit-frame-pointer -funwind-tables")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--strip-all -flto")

set(CMAKE_CURRENT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(INCLUDES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Includes)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ImGui)
set(IMGUI_BACKENDS_DIR ${IMGUI_DIR}/backends)
set(IMGUI_FONTS_DIR ${IMGUI_DIR}/fonts)
set(DOBBY_DIR ${INCLUDES_DIR}/Dobby)
set(FRIDA_DIR ${INCLUDES_DIR}/Frida)

include_directories(
    ${CMAKE_CURRENT_DIR}
    ${INCLUDES_DIR}
    ${IMGUI_DIR}
    ${IMGUI_BACKENDS_DIR}
    ${IMGUI_FONTS_DIR}
    ${DOBBY_DIR}/include
    ${INCLUDES_DIR}/json
    ${INCLUDES_DIR}/KittyMemory
    ${INCLUDES_DIR}/xdl/include
    ${INCLUDES_DIR}/il2cpp
    ${FRIDA_DIR}/${ANDROID_ABI}
    ${FRIDA_DIR}/gumpp
)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_BACKENDS_DIR}/imgui_impl_android.cpp
    ${IMGUI_BACKENDS_DIR}/imgui_impl_opengl3.cpp
)

set(DOBBY_GENERATE_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(BUILD_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(${DOBBY_DIR})
add_subdirectory(${INCLUDES_DIR}/xdl)
add_subdirectory(${INCLUDES_DIR}/asmjit Includes/asmjit)

add_library(kitty STATIC
    ${INCLUDES_DIR}/KittyMemory/KittyMemory.cpp
    ${INCLUDES_DIR}/KittyMemory/KittyUtils.cpp
    ${INCLUDES_DIR}/KittyMemory/MemoryBackup.cpp
    ${INCLUDES_DIR}/KittyMemory/MemoryPatch.cpp
)

add_library(gumpp STATIC
    ${FRIDA_DIR}/gumpp/backtracer.cpp
    ${FRIDA_DIR}/gumpp/interceptor.cpp
    ${FRIDA_DIR}/gumpp/invocationlistener.cpp
    ${FRIDA_DIR}/gumpp/returnaddress.cpp
    ${FRIDA_DIR}/gumpp/runtime.cpp
    ${FRIDA_DIR}/gumpp/symbolutil.cpp
)

add_library(Demo SHARED
    native-lib.cpp
    ${INCLUDES_DIR}/il2cpp/il2cpp_dump.cpp
)

target_link_libraries(Demo
    android
    log
    EGL
    GLESv2
    GLESv3
    imgui
    dobby
    kitty
    xdl
    asmjit
    gumpp
    ${FRIDA_DIR}/${ANDROID_ABI}/frida-gum.a
)
