cmake_minimum_required (VERSION 2.6)
project (openminecraft)
add_executable(openminecraft main.cpp controller.cpp map.cpp movement.cpp net.cpp ui.cpp renderer.cpp)

target_link_libraries (openminecraft SDL GL GLU)

