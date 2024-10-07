cmake_minimum_required(VERSION 3.20)

if (APPLE) # has to be set before the first project() or enable_language()
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version")
endif()

project(Zelda64Recompiled LANGUAGES C CXX)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-everything /W4")
endif()

# Opt out of constexpr mutex constructor on windows to prevent vcredist issues
if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR")
endif()

if (APPLE)
    enable_language(OBJC OBJCXX)

    # Check if VULKAN_SDK environment variable is set
    if(NOT DEFINED ENV{VULKAN_SDK})
        message(FATAL_ERROR "VULKAN_SDK environment variable is not set.")
    else()
        set(VULKAN_SDK $ENV{VULKAN_SDK})
    endif()

    # Define paths to Vulkan loader and MoltenVK libraries
    set(VULKAN_LOADER_PATH "${VULKAN_SDK}/lib/libvulkan.dylib")
    set(MOLTENVK_PATH "${VULKAN_SDK}/lib/libMoltenVK.dylib")

    # Ensure the Vulkan loader and MoltenVK libraries exist
    if(NOT EXISTS "${VULKAN_LOADER_PATH}")
        message(FATAL_ERROR "Vulkan loader not found at ${VULKAN_LOADER_PATH}")
    endif()

    if(NOT EXISTS "${MOLTENVK_PATH}")
        message(FATAL_ERROR "MoltenVK not found at ${MOLTENVK_PATH}")
    endif()
endif()

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if (WIN32)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/lib/")
endif()

set(RT64_STATIC TRUE)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/rt64 ${CMAKE_BINARY_DIR}/rt64)

# set(BUILD_SHARED_LIBS_SAVED "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)
SET(LUNASVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/lunasvg)
# set(BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS_SAVED}")
SET(ENABLE_SVG_PLUGIN ON CACHE BOOL "" FORCE)
SET(RMLUI_TESTS_ENABLED OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/RmlUi)

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime)

target_include_directories(rt64 PRIVATE ${CMAKE_BINARY_DIR}/rt64/src)

# RecompiledFuncs - Library containing the primary recompiler output
add_library(RecompiledFuncs STATIC)

target_compile_options(RecompiledFuncs PRIVATE
    # -Wno-unused-but-set-variable
    -fno-strict-aliasing
    -Wno-unused-variable
    -Wno-implicit-function-declaration
)

target_include_directories(RecompiledFuncs PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/ultramodern/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/librecomp/include
)

file(GLOB FUNC_C_SOURCES ${CMAKE_SOURCE_DIR}/RecompiledFuncs/*.c)
file(GLOB FUNC_CXX_SOURCES ${CMAKE_SOURCE_DIR}/RecompiledFuncs/*.cpp)

target_sources(RecompiledFuncs PRIVATE ${FUNC_C_SOURCES} ${FUNC_CXX_SOURCES})

# PatchesLib - Library containing the recompiled output for any custom function patches
add_library(PatchesLib STATIC)

target_compile_options(PatchesLib PRIVATE
    # -Wno-unused-but-set-variable
    -fno-strict-aliasing
    -Wno-unused-variable
    -Wno-implicit-function-declaration
)

target_include_directories(PatchesLib PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/ultramodern/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/librecomp/include
)

target_sources(PatchesLib PRIVATE
    ${CMAKE_SOURCE_DIR}/RecompiledPatches/patches.c
    ${CMAKE_SOURCE_DIR}/RecompiledPatches/patches_bin.c
)

set_source_files_properties(${CMAKE_SOURCE_DIR}/RecompiledPatches/patches.c PROPERTIES COMPILE_FLAGS -fno-strict-aliasing)

# Build patches elf
if(NOT DEFINED PATCHES_C_COMPILER)
    set(PATCHES_C_COMPILER clang)
endif()

if(NOT DEFINED PATCHES_LD)
    set(PATCHES_LD ld.lld)
endif()

if(NOT DEFINED PATCHES_OBJCOPY)
    set(PATCHES_OBJCOPY llvm-objcopy)
endif()

add_custom_target(PatchesBin
    COMMAND ${CMAKE_COMMAND} -E env CC=${PATCHES_C_COMPILER} LD=${PATCHES_LD} OBJCOPY=${PATCHES_OBJCOPY} make
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/patches
    BYPRODUCTS ${CMAKE_SOURCE_DIR}/patches/patches.elf
)

# Generate patches_bin.c from patches.bin
add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/RecompiledPatches/patches_bin.c
    COMMAND file_to_c ${CMAKE_SOURCE_DIR}/patches/patches.bin mm_patches_bin ${CMAKE_SOURCE_DIR}/RecompiledPatches/patches_bin.c ${CMAKE_SOURCE_DIR}/RecompiledPatches/patches_bin.h 
    DEPENDS ${CMAKE_SOURCE_DIR}/patches/patches.bin
)

# Recompile patches elf into patches.c and patches.bin
add_custom_command(OUTPUT
                       ${CMAKE_SOURCE_DIR}/patches/patches.bin
                       ${CMAKE_SOURCE_DIR}/RecompiledPatches/patches.c
                       ${CMAKE_SOURCE_DIR}/RecompiledPatches/recomp_overlays.inl
                       ${CMAKE_SOURCE_DIR}/RecompiledPatches/funcs.h
                   # TODO: Look into why modifying patches requires two builds to take
                   COMMAND ./N64Recomp patches.toml
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                   DEPENDS ${CMAKE_SOURCE_DIR}/patches/patches.elf
)

# Download controller db file for controller support via SDL2
set(GAMECONTROLLERDB_COMMIT "b1e4090b3d4266e55feb0793efa35792e05faf66")
set(GAMECONTROLLERDB_URL "https://raw.githubusercontent.com/gabomdq/SDL_GameControllerDB/${GAMECONTROLLERDB_COMMIT}/gamecontrollerdb.txt")

file(DOWNLOAD ${GAMECONTROLLERDB_URL} ${CMAKE_SOURCE_DIR}/gamecontrollerdb.txt
     TLS_VERIFY ON)

add_custom_target(DownloadGameControllerDB
    DEPENDS ${CMAKE_SOURCE_DIR}/gamecontrollerdb.txt)

# Main executable
add_executable(Zelda64Recompiled MACOSX_BUNDLE)
add_dependencies(Zelda64Recompiled DownloadGameControllerDB)

set (SOURCES
    ${CMAKE_SOURCE_DIR}/src/main/main.cpp
    ${CMAKE_SOURCE_DIR}/src/main/support.cpp
    ${CMAKE_SOURCE_DIR}/src/main/register_overlays.cpp
    ${CMAKE_SOURCE_DIR}/src/main/register_patches.cpp
    ${CMAKE_SOURCE_DIR}/src/main/rt64_render_context.cpp

    ${CMAKE_SOURCE_DIR}/src/game/input.cpp
    ${CMAKE_SOURCE_DIR}/src/game/controls.cpp
    ${CMAKE_SOURCE_DIR}/src/game/config.cpp
    ${CMAKE_SOURCE_DIR}/src/game/scene_table.cpp
    ${CMAKE_SOURCE_DIR}/src/game/debug.cpp
    ${CMAKE_SOURCE_DIR}/src/game/quicksaving.cpp
    ${CMAKE_SOURCE_DIR}/src/game/recomp_api.cpp

    ${CMAKE_SOURCE_DIR}/src/ui/ui_renderer.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ui_launcher.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ui_config.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ui_color_hack.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ui_rml_hacks.cpp

    ${CMAKE_SOURCE_DIR}/rsp/aspMain.cpp
    ${CMAKE_SOURCE_DIR}/rsp/njpgdspMain.cpp

    ${CMAKE_SOURCE_DIR}/lib/RmlUi/Backends/RmlUi_Platform_SDL.cpp
)

if (APPLE)
    list(APPEND SOURCES ${CMAKE_SOURCE_DIR}/src/main/support_apple.mm)
endif()

target_include_directories(Zelda64Recompiled PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/concurrentqueue
    ${CMAKE_SOURCE_DIR}/lib/GamepadMotionHelpers
    ${CMAKE_SOURCE_DIR}/lib/RmlUi/Include
    ${CMAKE_SOURCE_DIR}/lib/RmlUi/Backends
    ${CMAKE_SOURCE_DIR}/lib/rt64/src/contrib
    ${CMAKE_SOURCE_DIR}/lib/rt64/src/contrib/hlslpp/include
    ${CMAKE_SOURCE_DIR}/lib/rt64/src/contrib/dxc/inc
    ${CMAKE_SOURCE_DIR}/lib/rt64/src
    ${CMAKE_SOURCE_DIR}/lib/rt64/src/rhi
    ${CMAKE_SOURCE_DIR}/lib/rt64/src/render
    ${CMAKE_SOURCE_DIR}/lib/sse2neon
    ${CMAKE_SOURCE_DIR}/lib/freetype-windows-binaries/include
    ${CMAKE_SOURCE_DIR}/lib/rt64/src/contrib/nativefiledialog-extended/src/include
    ${CMAKE_BINARY_DIR}/shaders
    ${CMAKE_CURRENT_BINARY_DIR}
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
    target_compile_options(Zelda64Recompiled PRIVATE
        -march=nehalem
        -fno-strict-aliasing
        -fms-extensions
    )
else()
    target_compile_options(Zelda64Recompiled PRIVATE
        -fno-strict-aliasing
        -fms-extensions
    )
endif()

if (MSVC)
    # Disable identical code folding, since this breaks mod function patching as multiple functions can get merged into one.
    target_link_options(Zelda64Recompiled PRIVATE /OPT:NOICF)
endif()

if (WIN32)
    include(FetchContent)

    if (DEFINED ENV{SDL2_VERSION})
        set(SDL2_VERSION $ENV{SDL2_VERSION})
    else()
        set(SDL2_VERSION "2.30.3")
    endif()

    # Fetch SDL2 on windows
    FetchContent_Declare(
        sdl2
        URL https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VERSION}/SDL2-devel-${SDL2_VERSION}-VC.zip
    )
    FetchContent_MakeAvailable(sdl2)
    target_include_directories(Zelda64Recompiled PRIVATE
        ${sdl2_SOURCE_DIR}/include
    )
    target_link_directories(Zelda64Recompiled PRIVATE
        ${sdl2_SOURCE_DIR}/lib/x64
    )

    # Copy SDL2 and dxc DLLs to output folder as post build step
    add_custom_command(TARGET Zelda64Recompiled POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${sdl2_SOURCE_DIR}/lib/x64/SDL2.dll"
            "${CMAKE_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/x64/dxil.dll"
            "${CMAKE_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/x64/dxcompiler.dll"
            $<TARGET_FILE_DIR:Zelda64Recompiled>)

    set_target_properties(
        Zelda64Recompiled
        PROPERTIES
            LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
            LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
            LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
            LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )

    target_sources(Zelda64Recompiled PRIVATE ${CMAKE_SOURCE_DIR}/icons/app.rc)
    target_link_libraries(Zelda64Recompiled PRIVATE SDL2)
endif()

if (APPLE)
    find_package(SDL2 REQUIRED)
    target_include_directories(Zelda64Recompiled PRIVATE ${SDL2_INCLUDE_DIRS})

    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)

    target_link_libraries(Zelda64Recompiled PRIVATE ${CMAKE_DL_LIBS} Threads::Threads SDL2::SDL2)

    # Set bundle properties
    set_target_properties(Zelda64Recompiled PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME "Zelda64Recompiled"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.zelda64recompiled"
            MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
            MACOSX_BUNDLE_ICON_FILE "AppIcon.icns"
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_BINARY_DIR}/Info.plist
    )

    set(ICON_SOURCE ${CMAKE_SOURCE_DIR}/icons/512.png)
    set(ICONSET_DIR ${CMAKE_BINARY_DIR}/AppIcon.iconset)
    set(ICNS_FILE ${CMAKE_BINARY_DIR}/resources/AppIcon.icns)

    # Create iconset directory and add PNG file
    add_custom_command(
            OUTPUT ${ICONSET_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ICONSET_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy ${ICON_SOURCE} ${ICONSET_DIR}/icon_512x512.png
            COMMAND ${CMAKE_COMMAND} -E copy ${ICON_SOURCE} ${ICONSET_DIR}/icon_512x512@2x.png
            COMMAND touch ${ICONSET_DIR}
            COMMENT "Creating iconset directory and copying PNG file"
    )

    # Convert iconset to icns
    add_custom_command(
            OUTPUT ${ICNS_FILE}
            DEPENDS ${ICONSET_DIR}
            COMMAND iconutil -c icns ${ICONSET_DIR} -o ${ICNS_FILE}
            COMMENT "Converting iconset to icns"
    )

    # Custom target to ensure icns creation
    add_custom_target(create_icns ALL DEPENDS ${ICNS_FILE})

    # Set source file properties for the resulting icns file
    set_source_files_properties(${ICNS_FILE} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
    )

    # Add the icns file to the executable target
    target_sources(Zelda64Recompiled PRIVATE ${ICNS_FILE})

    # Ensure Zelda64Recompiled depends on create_icns
    add_dependencies(Zelda64Recompiled create_icns)

    # Configure Info.plist
    configure_file(${CMAKE_SOURCE_DIR}/.github/macos/Info.plist.in ${CMAKE_BINARY_DIR}/Info.plist @ONLY)

    # Install the app bundle
    install(TARGETS Zelda64Recompiled BUNDLE DESTINATION .)

    # Copy required frameworks to bundle
    target_link_libraries(Zelda64Recompiled PRIVATE ${MOLTENVK_PATH} ${VULKAN_LOADER_PATH})

    # Define the path to the entitlements file
    set(ENTITLEMENTS_FILE ${CMAKE_SOURCE_DIR}/.github/macos/entitlements.plist)

    # Ensure the entitlements file exists
    if(NOT EXISTS ${ENTITLEMENTS_FILE})
        message(FATAL_ERROR "Entitlements file not found at ${ENTITLEMENTS_FILE}")
    endif()

    # Post-build steps for macOS bundle
    add_custom_command(TARGET Zelda64Recompiled POST_BUILD
        # Copy and fix frameworks first
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/.github/macos/fixup_bundle.cmake

        # Copy all resources
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/temp_assets
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/temp_assets/scss
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/temp_assets $<TARGET_BUNDLE_DIR:Zelda64Recompiled>/Contents/Resources/assets
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/temp_assets

        # Copy Vulkan ICD files
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_BUNDLE_DIR:Zelda64Recompiled>/Contents/Resources/vulkan/icd.d
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/.github/macOS/MoltenVK_icd.json $<TARGET_BUNDLE_DIR:Zelda64Recompiled>/Contents/Resources/vulkan/icd.d/

        # Copy controller database
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/gamecontrollerdb.txt $<TARGET_BUNDLE_DIR:Zelda64Recompiled>/Contents/Resources/

        # Set RPATH
        COMMAND install_name_tool -add_rpath "@executable_path/../Frameworks/" $<TARGET_BUNDLE_DIR:Zelda64Recompiled>/Contents/MacOS/Zelda64Recompiled

        # Apply JIT workaround
        COMMAND ${CMAKE_COMMAND} -E echo "Applying JIT compilation workaround"
        COMMAND /bin/bash -c "printf '\\x07' | dd of=$<TARGET_FILE:Zelda64Recompiled> bs=1 seek=160 count=1 conv=notrunc"

        # Finally sign the whole bundle with runtime option and entitlements
        COMMAND codesign --deep --force --sign - --entitlements ${ENTITLEMENTS_FILE} $<TARGET_BUNDLE_DIR:Zelda64Recompiled>

        COMMENT "Performing post-build steps for macOS bundle"
        VERBATIM
    )
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(SDL2 REQUIRED)
    find_package(X11 REQUIRED)

    # Generate icon_bytes.c from the app icon PNG.
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/icon_bytes.c ${CMAKE_CURRENT_BINARY_DIR}/icon_bytes.h
        COMMAND file_to_c ${CMAKE_SOURCE_DIR}/icons/512.png icon_bytes ${CMAKE_CURRENT_BINARY_DIR}/icon_bytes.c ${CMAKE_CURRENT_BINARY_DIR}/icon_bytes.h 
        DEPENDS ${CMAKE_SOURCE_DIR}/icons/512.png
    )
    target_sources(Zelda64Recompiled PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/icon_bytes.c)

    message(STATUS "SDL2_FOUND = ${SDL2_FOUND}")
    message(STATUS "SDL2_INCLUDE_DIRS = ${SDL2_INCLUDE_DIRS}")

    target_include_directories(Zelda64Recompiled PRIVATE ${SDL2_INCLUDE_DIRS})

    message(STATUS "X11_FOUND = ${X11_FOUND}")
    message(STATUS "X11_Xrandr_FOUND = ${X11_Xrandr_FOUND}")
    message(STATUS "X11_INCLUDE_DIR = ${X11_INCLUDE_DIR}")
    message(STATUS "X11_LIBRARIES = ${X11_LIBRARIES}")

    target_include_directories(Zelda64Recompiled PRIVATE ${X11_INCLUDE_DIR} ${X11_Xrandr_INCLUDE_PATH})
    target_link_libraries(Zelda64Recompiled PRIVATE ${X11_LIBRARIES} ${X11_Xrandr_LIB})

    find_package(Freetype REQUIRED)

    message(STATUS "FREETYPE_FOUND = ${FREETYPE_FOUND}")
    message(STATUS "FREETYPE_INCLUDE_DIRS = ${FREETYPE_INCLUDE_DIRS}")
    message(STATUS "FREETYPE_LIBRARIES = ${FREETYPE_LIBRARIES}")

    include_directories(${FREETYPE_LIBRARIES})
    target_link_libraries(Zelda64Recompiled PRIVATE ${FREETYPE_LIBRARIES} SDL2::SDL2)

    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)

    target_link_libraries(Zelda64Recompiled PRIVATE "-latomic -static-libstdc++" ${CMAKE_DL_LIBS} Threads::Threads)
endif()

target_link_libraries(Zelda64Recompiled PRIVATE
    PatchesLib
    RecompiledFuncs
    librecomp
    ultramodern
    rt64
    RmlCore
    RmlDebugger
    nfd
    lunasvg
)

# TODO fix the rt64 CMake script so that this doesn't need to be duplicated here
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
    set (DXC "${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/x64/dxc.exe")
    add_compile_definitions(NOMINMAX)
else()
    if (APPLE)
        # Apple's binary is universal, so it'll work on both x86_64 and arm64
        set (DXC "DYLD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/lib/arm64" "${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/arm64/dxc-macos")
    else()
        if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
            set (DXC "LD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/lib/x64" "${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/x64/dxc")
        else()
            set (DXC "LD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/lib/arm64" "${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/arm64/dxc-linux")
        endif()
    endif()
endif()

build_vertex_shader(Zelda64Recompiled "shaders/InterfaceVS.hlsl" "shaders/InterfaceVS.hlsl")
build_pixel_shader(Zelda64Recompiled "shaders/InterfacePS.hlsl" "shaders/InterfacePS.hlsl")

target_sources(Zelda64Recompiled PRIVATE ${SOURCES})

set_property(TARGET Zelda64Recompiled PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

