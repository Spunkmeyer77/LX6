# LXR sound engine (from the open-source Erica Synths LXR firmware), built for desktop.
# SPDX-License-Identifier: GPL-3.0-only

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/host/sources.txt" LX6_ENGINE_SOURCES)
list(TRANSFORM LX6_ENGINE_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")

add_library(lxr_engine STATIC ${LX6_ENGINE_SOURCES})

target_include_directories(lxr_engine
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/host/stub"
        "${CMAKE_CURRENT_SOURCE_DIR}/lxr/Inc"
        "${CMAKE_CURRENT_SOURCE_DIR}/lxr/Inc/Soundengine"
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/host")

target_compile_definitions(lxr_engine PRIVATE LX6_HOST=1 __fp16=float)
set_target_properties(lxr_engine PROPERTIES POSITION_INDEPENDENT_CODE ON C_STANDARD 11)

if(MSVC AND NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "The LXR engine needs clang on Windows: configure with -T ClangCL")
endif()

if(WIN32)
    target_compile_definitions(lxr_engine PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
endif()
if(MSVC)
    target_compile_options(lxr_engine PRIVATE /w /O2)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    # firmware code: keep it quiet, keep float maths IEEE-safe but fast enough
    target_compile_options(lxr_engine PRIVATE -w -O2 -fno-strict-aliasing)
endif()
