include(BundleUtilities)

# Check for pkgx installation
find_program(PKGX_EXECUTABLE pkgx)

# Xcode generator puts the build type in the build directory
set(BUILD_PREFIX "")
if (CMAKE_GENERATOR STREQUAL "Xcode")
    set(BUILD_PREFIX "${CMAKE_BUILD_TYPE}/")
endif()

# Use generator expressions to get the absolute path to the bundle
set(APPS "${BUILD_PREFIX}Zelda64Recompiled.app/Contents/MacOS/Zelda64Recompiled")

# Set up framework search paths
set(DIRS "${BUILD_PREFIX}Zelda64Recompiled.app/Contents/Frameworks")

# Detect if we're using pkgx
if(PKGX_EXECUTABLE)
    message(STATUS "pkgx detected, adding pkgx directories to framework search path")
    list(APPEND DIRS "$ENV{HOME}/.pkgx/")
endif()

# Convert all paths to absolute paths
file(REAL_PATH ${APPS} APPS)

set(RESOLVED_DIRS "")
foreach(DIR IN LISTS DIRS)
    # Handle home directory expansion
    string(REPLACE "~" "$ENV{HOME}" DIR "${DIR}")
    # Convert to absolute path, but don't fail if directory doesn't exist
    if(EXISTS "${DIR}")
        file(REAL_PATH "${DIR}" RESOLVED_DIR)
        list(APPEND RESOLVED_DIRS "${RESOLVED_DIR}")
    endif()
endforeach()

# Debug output
message(STATUS "Bundle fixup paths:")
message(STATUS "  App: ${APPS}")
message(STATUS "  Search dirs: ${RESOLVED_DIRS}")

# Fix up the bundle
fixup_bundle("${APPS}" "" "${RESOLVED_DIRS}")