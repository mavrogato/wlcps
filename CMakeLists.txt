
cmake_minimum_required(VERSION 3.14)

set(PROJ "wlcps")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_COMPILER "icpx")
set(CMAKE_CXX_FLAGS "-O3")

project(${PROJ})

find_package(IntelDPCPP REQUIRED)

add_executable(${PROJ}
  main.cc)

target_compile_options(${PROJ}
  PRIVATE
  -Wall
  -std=c++20)

target_link_libraries(${PROJ}
  PRIVATE
  wayland-client)

add_custom_target(run
  DEPENDS ${PROJ}
  COMMAND ./${PROJ})
#  COMMAND WAYLAND_DEBUG=1 ./${PROJ})
