cmake_minimum_required (VERSION 2.6)
project (cubit)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

find_package( SDL )
find_package( SDL_mixer )
find_package( OpenGL )
find_package( Boost COMPONENTS program_options )
find_package( Sqlite3 )

add_executable(cubit main.cpp controller.cpp map.cpp movement.cpp net.cpp ui.cpp renderer.cpp)

include_directories( ${SDL_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${SQLITE3_INCLUDE_DIR})
target_link_libraries (cubit ${SDL_LIBRARY} ${SDLMIXER_LIBRARY} ${OPENGL_LIBRARIES} ${Boost_LIBRARIES} ${SQLITE3_LIBRARIES})




