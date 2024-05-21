try_compile(SUPPORTS_SSE42
  "${CMAKE_BINARY_DIR}"
  "${CMAKE_SOURCE_DIR}/cmake/checks/cpu_sse42.cpp"
)