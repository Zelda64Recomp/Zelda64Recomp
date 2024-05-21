if(NOT DEFINED SUPPORTS_NEON)
  try_compile(SUPPORTS_NEON
    "${CMAKE_BINARY_DIR}"
    "${CMAKE_SOURCE_DIR}/cmake/checks/cpu_neon.cpp"
  )
endif()

message(STATUS "SUPPORTS NEON: ${SUPPORTS_NEON}")
