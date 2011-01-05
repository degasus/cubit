cmake_minimum_required (VERSION 2.6)
project (cubit)
add_executable(cubit main.cpp controller.cpp map.cpp movement.cpp net.cpp ui.cpp renderer.cpp)

target_link_libraries (cubit SDL GL GLU boost_program_options sqlite3)

