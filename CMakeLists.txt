cmake_minimum_required (VERSION 2.6)
project (cubit)

find_package( SDL )
find_package( SDL_mixer )
find_package( SDL_image )
find_package( OpenGL )
find_package( Boost COMPONENTS program_options filesystem system)
find_package( PkgConfig )
pkg_check_modules(PKG REQUIRED sqlite3 bullet)

FIND_PATH(LZO_INCLUDE_DIR lzo1x.h
	/usr/include/lzo/
	/usr/local/include/lzo/
)

FIND_LIBRARY(LZO_LIBRARIES NAMES lzo2
	/usr/lib
	/usr/local/lib
)

ADD_DEFINITIONS(-DCMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")


add_executable(cubit main.cpp controller.cpp map.cpp movement.cpp net.cpp ui.cpp renderer.cpp GLDebugDrawer.cpp)

include_directories( ${SDL_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${SDLIMAGE_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${PKG_INCLUDE_DIRS} ${LZO_INCLUDE_DIR})
target_link_libraries (cubit ${SDL_LIBRARY} ${SDLMIXER_LIBRARY} ${SDLIMAGE_LIBRARY} ${OPENGL_LIBRARIES} ${Boost_LIBRARIES} ${PKG_LIBRARIES} ${LZO_LIBRARIES})



INSTALL( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/cubit DESTINATION games)

FILE(GLOB tex "${CMAKE_CURRENT_SOURCE_DIR}/tex/*" )
INSTALL( FILES ${tex} DESTINATION share/games/cubit/tex)

FILE(GLOB soundfx "${CMAKE_CURRENT_SOURCE_DIR}/sound/fx/*" )
INSTALL( FILES ${soundfx} DESTINATION share/games/cubit/sound/fx)

FILE(GLOB soundmusic "${CMAKE_CURRENT_SOURCE_DIR}/sound/music/*" )
INSTALL( FILES ${soundmusic} DESTINATION share/games/cubit/sound/music)

INSTALL( FILES logo/cubit.svg DESTINATION share/icons/hicolor/scalable/apps)
INSTALL( FILES logo/cubit.desktop DESTINATION share/applications)
