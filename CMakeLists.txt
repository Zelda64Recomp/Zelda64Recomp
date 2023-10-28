cmake_minimum_required(VERSION 3.20)
project(MMRecomp)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(${CMAKE_SOURCE_DIR}/../mupen_rt64/mupen64plus-video-rt64 ${CMAKE_BINARY_DIR}/rt64)

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
    
    ${CMAKE_SOURCE_DIR}/rsp/aspMain.cpp
    ${CMAKE_SOURCE_DIR}/rsp/njpgdspMain.cpp
)

add_executable(MMRecomp)

target_include_directories(MMRecomp PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/thirdparty
    ${CMAKE_SOURCE_DIR}/../mupen_rt64/mupen64plus-win32-deps/SDL2-2.26.3/include
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
)

target_sources(MMRecomp PRIVATE ${SOURCES})

set_property(TARGET MMRecomp PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_custom_command(TARGET MMRecomp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/${RT64_DLL}
        ${CMAKE_SOURCE_DIR}
    MAIN_DEPENDENCY ${CMAKE_BINARY_DIR}/${RT64_DLL})
