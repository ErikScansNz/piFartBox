find_package(Threads REQUIRED)
find_library(ALSA_LIBRARY NAMES asound REQUIRED)

add_library(pi_fartbox_platform_linux STATIC
  src/linux_host.cpp
)

target_include_directories(pi_fartbox_platform_linux
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pi_fartbox_platform_linux
  PUBLIC
    Threads::Threads
    ${ALSA_LIBRARY}
)
