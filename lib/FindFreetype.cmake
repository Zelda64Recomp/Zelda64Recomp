set(FREETYPE_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/lib/freetype-windows-binaries/include)
set(FREETYPE_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/freetype-windows-binaries/release static/vs2015-2022/win64/freetype.lib")
add_library(Freetype::Freetype STATIC IMPORTED)
set_target_properties(Freetype::Freetype PROPERTIES
	IMPORTED_LOCATION ${FREETYPE_LIBRARIES}
)
target_include_directories(Freetype::Freetype INTERFACE
	${FREETYPE_INCLUDE_DIRS}
)
