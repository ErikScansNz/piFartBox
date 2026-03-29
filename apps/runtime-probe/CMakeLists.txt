add_executable(pi_fartbox_runtime_probe
  src/main.cpp
)

target_link_libraries(pi_fartbox_runtime_probe
  PRIVATE
    pi_fartbox_controller_mapper
    pi_fartbox_control_api
    pi_fartbox_engine_core
    pi_fartbox_platform_linux
    pi_fartbox_session_manager
    pi_fartbox_zynthian_adapter
)

target_link_options(pi_fartbox_runtime_probe
  PRIVATE
    -static-libstdc++
    -static-libgcc
)
