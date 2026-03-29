add_library(pi_fartbox_platform_linux STATIC
  src/linux_host.cpp
)

target_include_directories(pi_fartbox_platform_linux
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
