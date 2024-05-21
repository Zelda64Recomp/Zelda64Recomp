if(NOT DEFINED SUPPORTS_SSE42)
  try_compile(SUPPORTS_SSE42
    "${CMAKE_BINARY_DIR}"
    "${CMAKE_SOURCE_DIR}/cmake/checks/cpu_sse42.cpp"
  )
endif()

message(STATUS "SUPPORTS SSE4.2 = ${SUPPORTS_SSE42}")
