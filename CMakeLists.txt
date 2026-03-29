cmake_minimum_required(VERSION 3.20)
project(piFartBox VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(PI_FARTBOX_ENABLE_ZYNTHIAN "Build Zynthian integration targets" ON)
option(PI_FARTBOX_ENABLE_CONTROL_API "Build control API targets" ON)
option(PI_FARTBOX_BUILD_RUNTIME_PROBE "Build the runtime probe executable" ON)

add_subdirectory(engine)
add_subdirectory(session-manager)
add_subdirectory(platform/linux)
add_subdirectory(controller-mapper)
add_subdirectory(integrations/zynthian)
add_subdirectory(control-api)

if(PI_FARTBOX_BUILD_RUNTIME_PROBE)
  add_subdirectory(apps/runtime-probe)
endif()
