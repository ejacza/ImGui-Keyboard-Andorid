file(GLOB ASMJIT_SRC
    core/*.cpp
    support/*.cpp
    arm/*.cpp
    x86/*.cpp
    ujit/*.cpp
)
add_library(asmjit STATIC ${ASMJIT_SRC})
target_include_directories(asmjit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_definitions(asmjit PUBLIC ASMJIT_STATIC ASMJIT_NO_FOREIGN=1)
target_compile_options(asmjit PRIVATE -O3 -fvisibility=hidden -ffunction-sections -fdata-sections)
if(ANDROID)
    target_compile_definitions(asmjit PUBLIC ASMJIT_NO_SHM_OPEN=1)
endif()