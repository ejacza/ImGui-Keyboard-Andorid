LOCAL_PATH := $(call my-dir)

INCLUDES_DIR := $(LOCAL_PATH)/Includes
IMGUI_DIR := $(LOCAL_PATH)/ImGui
IMGUI_BACKENDS_DIR := $(IMGUI_DIR)/backends
DOBBY_DIR := $(INCLUDES_DIR)/Dobby


include $(CLEAR_VARS)
LOCAL_MODULE := imgui
LOCAL_SRC_FILES := \
    $(IMGUI_DIR)/imgui.cpp \
    $(IMGUI_DIR)/imgui_demo.cpp \
    $(IMGUI_DIR)/imgui_draw.cpp \
    $(IMGUI_DIR)/imgui_tables.cpp \
    $(IMGUI_DIR)/imgui_widgets.cpp \
    $(IMGUI_BACKENDS_DIR)/imgui_impl_android.cpp \
    $(IMGUI_BACKENDS_DIR)/imgui_impl_opengl3.cpp
LOCAL_C_INCLUDES := \
    $(IMGUI_DIR) \
    $(IMGUI_BACKENDS_DIR) \
    $(IMGUI_DIR)/fonts
LOCAL_CPP_FEATURES := exceptions rtti
include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := kitty
LOCAL_SRC_FILES := \
    $(INCLUDES_DIR)/KittyMemory/KittyMemory.cpp \
    $(INCLUDES_DIR)/KittyMemory/KittyUtils.cpp \
    $(INCLUDES_DIR)/KittyMemory/MemoryBackup.cpp \
    $(INCLUDES_DIR)/KittyMemory/MemoryPatch.cpp
LOCAL_C_INCLUDES := \
    $(INCLUDES_DIR)/KittyMemory
LOCAL_CPP_FEATURES := exceptions rtti
include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := dobby
LOCAL_SRC_FILES := $(DOBBY_DIR)/$(TARGET_ARCH_ABI)/libdobby.a
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := Demo
LOCAL_SRC_FILES := native-lib.cpp
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(INCLUDES_DIR) \
    $(IMGUI_DIR) \
    $(IMGUI_BACKENDS_DIR) \
    $(IMGUI_DIR)/fonts \
    $(DOBBY_DIR)/include \
    $(INCLUDES_DIR)/json \
    $(INCLUDES_DIR)/KittyMemory
    LOCAL_STATIC_LIBRARIES := imgui dobby kitty
    LOCAL_LDLIBS := -landroid -llog -lEGL -lGLESv2 -lGLESv3
    LOCAL_CPPFLAGS := -std=c++20 -O3 -flto -fvisibility=hidden \
    -ffunction-sections -fdata-sections \
    -fomit-frame-pointer -funwind-tables
LOCAL_LDFLAGS := -Wl,--gc-sections -Wl,--strip-all -flto
LOCAL_CPP_FEATURES := exceptions rtti
include $(BUILD_SHARED_LIBRARY)