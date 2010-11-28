cmake_minimum_required (VERSION 2.6)
project (openminecraft)
add_executable(openminecraft test.cpp)

target_link_libraries (openminecraft SDL GL GLU)

