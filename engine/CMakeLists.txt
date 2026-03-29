add_library(pi_fartbox_engine_core STATIC
  src/device_registry.cpp
  src/device_schema.cpp
  src/engine_runtime.cpp
)

target_include_directories(pi_fartbox_engine_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
