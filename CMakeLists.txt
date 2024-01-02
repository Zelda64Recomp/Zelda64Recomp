cmake_minimum_required(VERSION 3.20)
project(MMRecomp)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/lib/")

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/RT64-HLE ${CMAKE_BINARY_DIR}/rt64)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/RmlUi)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/nativefiledialog-extended)

target_include_directories(rt64 PRIVATE ${CMAKE_BINARY_DIR}/rt64/src)
get_target_property(RT64_BASENAME rt64 OUTPUT_NAME)
set(RT64_DLL ${RT64_BASENAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

# RecompiledFuncs - Library containing the primary recompiler output
add_library(RecompiledFuncs STATIC)

target_compile_options(RecompiledFuncs PRIVATE
    # -Wno-unused-but-set-variable
    -fno-strict-aliasing
)

target_include_directories(RecompiledFuncs PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

file(GLOB FUNC_C_SOURCES ${CMAKE_SOURCE_DIR}/RecompiledFuncs/*.c)
file(GLOB FUNC_CXX_SOURCES ${CMAKE_SOURCE_DIR}/RecompiledFuncs/*.cpp)

target_sources(RecompiledFuncs PRIVATE ${FUNC_C_SOURCES} ${FUNC_CXX_SOURCES})

# PatchesLib - Library containing the recompiled output for any custom function patches
add_library(PatchesLib STATIC)

target_compile_options(PatchesLib PRIVATE
    # -Wno-unused-but-set-variable
    -fno-strict-aliasing
)

target_include_directories(PatchesLib PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_sources(PatchesLib PRIVATE
    ${CMAKE_SOURCE_DIR}/RecompiledPatches/patches.c
)

# Build patches elf
add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/patches/patches.elf
    COMMAND make
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/patches
    BYPRODUCTS ${CMAKE_SOURCE_DIR}/patches/patches.bin}
)

# Recompile patches elf into patches.c
add_custom_command(OUTPUT
                       ${CMAKE_SOURCE_DIR}/RecompiledPatches/patches.c
                       ${CMAKE_SOURCE_DIR}/RecompiledPatches/recomp_overlays.inl
                       ${CMAKE_SOURCE_DIR}/RecompiledPatches/funcs.h
                       ${CMAKE_SOURCE_DIR}/src/recomp/patch_loading.cpp
                   COMMAND RecompPort patches.toml && ${CMAKE_COMMAND} -E touch ${CMAKE_SOURCE_DIR}/src/recomp/patch_loading.cpp
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                   DEPENDS ${CMAKE_SOURCE_DIR}/patches/patches.elf
)

# Main executable
add_executable(MMRecomp)

set (SOURCES
    ${CMAKE_SOURCE_DIR}/ultramodern/audio.cpp
    ${CMAKE_SOURCE_DIR}/ultramodern/events.cpp
    ${CMAKE_SOURCE_DIR}/ultramodern/mesgqueue.cpp
    ${CMAKE_SOURCE_DIR}/ultramodern/misc_ultra.cpp
    ${CMAKE_SOURCE_DIR}/ultramodern/port_main.c
    ${CMAKE_SOURCE_DIR}/ultramodern/scheduler.cpp
    ${CMAKE_SOURCE_DIR}/ultramodern/task_win32.cpp
    ${CMAKE_SOURCE_DIR}/ultramodern/threads.cpp
    ${CMAKE_SOURCE_DIR}/ultramodern/timer.cpp
    ${CMAKE_SOURCE_DIR}/ultramodern/ultrainit.cpp

    ${CMAKE_SOURCE_DIR}/src/recomp/ai.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/cont.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/dp.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/eep.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/euc-jp.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/flash.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/math_routines.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/overlays.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/patch_loading.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/pak.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/pi.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/ultra_stubs.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/ultra_translation.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/print.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/recomp.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/rt64_layer.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/sp.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp/vi.cpp

    ${CMAKE_SOURCE_DIR}/src/main/main.cpp
    
    ${CMAKE_SOURCE_DIR}/src/game/input.cpp
    ${CMAKE_SOURCE_DIR}/src/game/controls.cpp

    ${CMAKE_SOURCE_DIR}/src/ui/ui_renderer.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ui_launcher.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ui_config.cpp
    
    ${CMAKE_SOURCE_DIR}/rsp/aspMain.cpp
    ${CMAKE_SOURCE_DIR}/rsp/njpgdspMain.cpp

    ${CMAKE_SOURCE_DIR}/lib/RmlUi/Backends/RmlUi_Platform_SDL.cpp
)

target_include_directories(MMRecomp PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/concurrentqueue
    ${CMAKE_SOURCE_DIR}/lib/RmlUi/Include
    ${CMAKE_SOURCE_DIR}/lib/RmlUi/Backends
    ${CMAKE_SOURCE_DIR}/lib/RT64-HLE/src/contrib
    ${CMAKE_SOURCE_DIR}/lib/RT64-HLE/src/contrib/mupen64plus-win32-deps/SDL2-2.26.3/include
    ${CMAKE_SOURCE_DIR}/lib/RT64-HLE/src/contrib/hlslpp/include
    ${CMAKE_SOURCE_DIR}/lib/RT64-HLE/src
    ${CMAKE_SOURCE_DIR}/lib/RT64-HLE/src/rhi
    ${CMAKE_SOURCE_DIR}/lib/RT64-HLE/src/render
    ${CMAKE_BINARY_DIR}/shaders
)

target_compile_options(MMRecomp PRIVATE
    -march=nehalem
    -fno-strict-aliasing
    -fms-extensions
)

target_link_directories(MMRecomp PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/RT64-HLE/src/contrib/mupen64plus-win32-deps/SDL2-2.26.3/lib/x64
)

target_link_libraries(MMRecomp PRIVATE
    PatchesLib
    RecompiledFuncs
    SDL2
    rt64
    RmlCore
    RmlDebugger
    nfd
)

# TODO fix the RT64 CMake script so that this doesn't need to be duplicated here
# For DXC
set (DXC_COMMON_OPTS "-I${PROJECT_SOURCE_DIR}/src")
set (DXC_DXIL_OPTS "-Wno-ignored-attributes")
set (DXC_SPV_OPTS "-spirv" "-fspv-target-env=vulkan1.0" "-fvk-use-dx-layout")
set (DXC_PS_OPTS "${DXC_COMMON_OPTS}" "-E" "PSMain" "-T ps_6_0" "-D DYNAMIC_RENDER_PARAMS")
set (DXC_VS_OPTS "${DXC_COMMON_OPTS}" "-E" "VSMain" "-T vs_6_0" "-D DYNAMIC_RENDER_PARAMS" "-fvk-invert-y")
set (DXC_CS_OPTS "${DXC_COMMON_OPTS}" "-E" "CSMain" "-T cs_6_0")
set (DXC_GS_OPTS "${DXC_COMMON_OPTS}" "-E" "GSMain" "-T gs_6_0")
set (DXC_RT_OPTS "${DXC_COMMON_OPTS}" "-D" "RT_SHADER" "-T" "lib_6_3" "-fspv-target-env=vulkan1.1spirv1.4" "-fspv-extension=SPV_KHR_ray_tracing" "-fspv-extension=SPV_EXT_descriptor_indexing")

if (${WIN32})
    set (DXC "${PROJECT_SOURCE_DIR}/lib/RT64-HLE/src/contrib/dxc/bin/x64/dxc.exe")
    add_compile_definitions(NOMINMAX)
else()
    set (DXC "LD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/lib/RT64-HLE/src/src/contrib/dxc/lib/x64" "${PROJECT_SOURCE_DIR}/src/contrib/dxc/bin/x64/dxc")
endif()

build_vertex_shader(MMRecomp "shaders/InterfaceVS.hlsl" "shaders/InterfaceVS.hlsl")
build_pixel_shader (MMRecomp "shaders/InterfacePS.hlsl" "shaders/InterfacePS.hlsl")

target_sources(MMRecomp PRIVATE ${SOURCES})

set_property(TARGET MMRecomp PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_custom_command(TARGET MMRecomp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/${RT64_DLL}
        ${CMAKE_SOURCE_DIR}
    MAIN_DEPENDENCY ${CMAKE_BINARY_DIR}/${RT64_DLL})
