cmake_minimum_required(VERSION 3.20)
project(MMRecomp)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(Freetype REQUIRED)

add_subdirectory(${CMAKE_SOURCE_DIR}/../mupen_rt64/mupen64plus-video-rt64 ${CMAKE_BINARY_DIR}/rt64)
add_subdirectory(${CMAKE_SOURCE_DIR}/thirdparty/RmlUi)

target_include_directories(rt64 PRIVATE ${CMAKE_BINARY_DIR}/rt64/src)
get_target_property(RT64_BASENAME rt64 OUTPUT_NAME)
set(RT64_DLL ${RT64_BASENAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

file(GLOB FUNC_C_SOURCES ${CMAKE_SOURCE_DIR}/RecompiledFuncs/*.c)
file(GLOB FUNC_CXX_SOURCES ${CMAKE_SOURCE_DIR}/RecompiledFuncs/*.cpp)

add_library(RecompiledFuncs STATIC)

target_compile_options(RecompiledFuncs PRIVATE
    # -Wno-unused-but-set-variable
    -fno-strict-aliasing
)

target_include_directories(RecompiledFuncs PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_sources(RecompiledFuncs PRIVATE ${FUNC_C_SOURCES} ${FUNC_CXX_SOURCES})

set (SOURCES
    ${CMAKE_SOURCE_DIR}/portultra/audio.cpp
    ${CMAKE_SOURCE_DIR}/portultra/events.cpp
    ${CMAKE_SOURCE_DIR}/portultra/mesgqueue.cpp
    ${CMAKE_SOURCE_DIR}/portultra/misc_ultra.cpp
    ${CMAKE_SOURCE_DIR}/portultra/port_main.c
    ${CMAKE_SOURCE_DIR}/portultra/scheduler.cpp
    ${CMAKE_SOURCE_DIR}/portultra/task_win32.cpp
    ${CMAKE_SOURCE_DIR}/portultra/threads.cpp
    ${CMAKE_SOURCE_DIR}/portultra/timer.cpp
    ${CMAKE_SOURCE_DIR}/portultra/ultrainit.cpp

    ${CMAKE_SOURCE_DIR}/src/ai.cpp
    ${CMAKE_SOURCE_DIR}/src/cont.cpp
    ${CMAKE_SOURCE_DIR}/src/dp.cpp
    ${CMAKE_SOURCE_DIR}/src/eep.cpp
    ${CMAKE_SOURCE_DIR}/src/euc-jp.cpp
    ${CMAKE_SOURCE_DIR}/src/flash.cpp
    ${CMAKE_SOURCE_DIR}/src/math_routines.cpp
    ${CMAKE_SOURCE_DIR}/src/overlays.cpp
    ${CMAKE_SOURCE_DIR}/src/pak.cpp
    ${CMAKE_SOURCE_DIR}/src/pi.cpp
    ${CMAKE_SOURCE_DIR}/src/portultra_stubs.cpp
    ${CMAKE_SOURCE_DIR}/src/portultra_translation.cpp
    ${CMAKE_SOURCE_DIR}/src/print.cpp
    ${CMAKE_SOURCE_DIR}/src/recomp.cpp
    ${CMAKE_SOURCE_DIR}/src/rt64_layer.cpp
    ${CMAKE_SOURCE_DIR}/src/sp.cpp
    ${CMAKE_SOURCE_DIR}/src/vi.cpp
    ${CMAKE_SOURCE_DIR}/src/main/main.cpp
    ${CMAKE_SOURCE_DIR}/src/ui.cpp
    
    ${CMAKE_SOURCE_DIR}/rsp/aspMain.cpp
    ${CMAKE_SOURCE_DIR}/rsp/njpgdspMain.cpp

    ${CMAKE_SOURCE_DIR}/thirdparty/RmlUi/Backends/RmlUi_Platform_SDL.cpp
)

add_executable(MMRecomp)

target_include_directories(MMRecomp PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/thirdparty
    ${CMAKE_SOURCE_DIR}/thirdparty/RmlUi/Include
    ${CMAKE_SOURCE_DIR}/thirdparty/RmlUi/Backends
    ${CMAKE_SOURCE_DIR}/../mupen_rt64/mupen64plus-win32-deps/SDL2-2.26.3/include
    ${CMAKE_SOURCE_DIR}/../mupen_rt64/mupen64plus-video-rt64/src
    ${CMAKE_SOURCE_DIR}/../mupen_rt64/mupen64plus-video-rt64/src/rhi
    ${CMAKE_SOURCE_DIR}/../mupen_rt64/mupen64plus-video-rt64/src/render
    ${CMAKE_BINARY_DIR}/shaders
)

target_compile_options(MMRecomp PRIVATE
    -march=nehalem
    -fno-strict-aliasing
    -fms-extensions
)

target_link_directories(MMRecomp PRIVATE
    ${CMAKE_SOURCE_DIR}/../mupen_rt64/mupen64plus-win32-deps/SDL2-2.26.3/lib/x64
)

target_link_libraries(MMRecomp PRIVATE
    RecompiledFuncs
    SDL2
    rt64
    Freetype::Freetype
    RmlCore
    RmlDebugger
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
    set (DXC "${PROJECT_SOURCE_DIR}/../mupen_rt64/mupen64plus-video-rt64/src/contrib/dxc/bin/x64/dxc.exe")
    add_compile_definitions(NOMINMAX)
else()
    set (DXC "LD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/../mupen_rt64/mupen64plus-video-rt64/src/contrib/dxc/lib/x64" "${PROJECT_SOURCE_DIR}/src/contrib/dxc/bin/x64/dxc")
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
