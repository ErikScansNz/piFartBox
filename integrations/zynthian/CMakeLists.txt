add_library(pi_fartbox_zynthian_adapter STATIC
  src/zynthian_adapter.cpp
)

target_include_directories(pi_fartbox_zynthian_adapter
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pi_fartbox_zynthian_adapter
  PUBLIC
    pi_fartbox_controller_mapper
    pi_fartbox_engine_core
    pi_fartbox_platform_linux
    pi_fartbox_session_manager
)
