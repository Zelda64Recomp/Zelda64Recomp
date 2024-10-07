include(BundleUtilities)

# Use generator expressions to get the absolute path to the bundle and frameworks
set(APPS "Zelda64Recompiled.app/Contents/MacOS/Zelda64Recompiled")
set(DIRS "Zelda64Recompiled.app/Contents/Frameworks")

# The fixup_bundle command needs an absolute path
file(REAL_PATH ${APPS} APPS)
file(REAL_PATH ${DIRS} DIRS)

fixup_bundle("${APPS}" "" "${DIRS}")