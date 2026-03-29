add_library(pi_fartbox_control_api STATIC
  src/control_server.cpp
)

target_include_directories(pi_fartbox_control_api
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pi_fartbox_control_api
  PUBLIC
    pi_fartbox_engine_core
    pi_fartbox_platform_linux
    pi_fartbox_session_manager
)
