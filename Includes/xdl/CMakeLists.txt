add_library(xdl STATIC
    xdl.c
    xdl_iterate.c
    xdl_linker.c
    xdl_lzma.c
    xdl_util.c
)

target_include_directories(xdl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(xdl PRIVATE
    -O3 -DNDEBUG -fvisibility=hidden -flto
    -ffunction-sections -fdata-sections
)
