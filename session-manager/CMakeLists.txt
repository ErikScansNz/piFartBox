add_library(pi_fartbox_session_manager STATIC
  src/session_manager.cpp
)

target_include_directories(pi_fartbox_session_manager
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pi_fartbox_session_manager
  PUBLIC
    pi_fartbox_engine_core
)
