add_library(pi_fartbox_controller_mapper STATIC
  src/controller_core.cpp
  src/slmkiii_mapper.cpp
)

target_include_directories(pi_fartbox_controller_mapper
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pi_fartbox_controller_mapper
  PUBLIC
    pi_fartbox_engine_core
    pi_fartbox_session_manager
)
