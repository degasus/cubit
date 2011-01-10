cmake_minimum_required (VERSION 2.6)
project (cubit)


set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

find_package( SDL )
find_package( SDL_mixer )
find_package( OpenGL )
find_package( Boost COMPONENTS program_options filesystem system)
find_package( Sqlite3 )



# set default build type
IF (NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release)
ENDIF (NOT CMAKE_BUILD_TYPE)

ADD_DEFINITIONS(-DCMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}" )


add_executable(cubit main.cpp controller.cpp map.cpp movement.cpp net.cpp ui.cpp renderer.cpp)

include_directories( ${SDL_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${SQLITE3_INCLUDE_DIR})
target_link_libraries (cubit ${SDL_LIBRARY} ${SDLMIXER_LIBRARY} ${OPENGL_LIBRARIES} ${Boost_LIBRARIES} ${SQLITE3_LIBRARIES})



INSTALL( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/cubit DESTINATION games)

FILE(GLOB tex "${CMAKE_CURRENT_SOURCE_DIR}/tex/*.bmp" )
INSTALL( FILES ${tex} DESTINATION share/games/cubit/tex)

FILE(GLOB soundfx "${CMAKE_CURRENT_SOURCE_DIR}/sound/fx/*.ogg" )
INSTALL( FILES ${soundfx} DESTINATION share/games/cubit/sound/fx)

FILE(GLOB soundmusic "${CMAKE_CURRENT_SOURCE_DIR}/sound/music/*.ogg" )
INSTALL( FILES ${soundmusic} DESTINATION share/games/cubit/sound/music)

INSTALL( FILES logo/cubit.png DESTINATION share/icons/hicolor/128x128/apps)
INSTALL( FILES logo/cubit.desktop DESTINATION share/applications)

