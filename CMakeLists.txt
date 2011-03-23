cmake_minimum_required (VERSION 2.6)
project (cubit)

find_package( SDL )
find_package( SDL_mixer )
find_package( SDL_image )
find_package( FTGL )
find_package( Freetype )
find_package( OpenGL )
find_package( Boost COMPONENTS program_options filesystem system )
#find_package( PkgConfig )
#pkg_check_modules(PKG REQUIRED sqlite3 bullet)

FIND_PATH(LZO_INCLUDE_DIR lzo/lzo1x.h
	HINTS
	$ENV{LZODIR}
	PATH_SUFFIXES include
	PATHS
	/usr/include/lzo/
	/usr/local/include/lzo/
)

FIND_LIBRARY(LZO_LIBRARIES NAMES lzo2
	HINTS
	$ENV{LZODIR}
	PATH_SUFFIXES lib
	PATHS
	/usr/include/lzo/
	/usr/local/include/lzo/
)

FIND_PATH(SQLITE_INCLUDE_DIR sqlite3.h
	HINTS
	$ENV{SQLITEDIR}
	PATH_SUFFIXES include
	PATHS
	/usr/include/
	/usr/local/include/
)

FIND_LIBRARY(SQLITE_LIBRARIES NAMES sqlite3
	HINTS
	$ENV{SQLITEDIR}
	PATH_SUFFIXES lib
	PATHS
	/usr/include/
	/usr/local/include/
)

FIND_PATH(BULLET_INCLUDE_DIR btBulletDynamicsCommon.h
	HINTS
	$ENV{BULLETDIR}
	PATH_SUFFIXES include/bullet
	PATHS
	/usr/include/
	/usr/local/include/
)

FIND_LIBRARY(BULLET_LIBRARIES_SOFTBODY NAMES BulletSoftBody BulletDynamics BulletCollision LinearMath
	HINTS
	$ENV{BULLETDIR}
	PATH_SUFFIXES lib
	PATHS
	/usr/include/
	/usr/local/include/
)
FIND_LIBRARY(BULLET_LIBRARIES_DYNAMICS NAMES BulletDynamics 
	HINTS
	$ENV{BULLETDIR}
	PATH_SUFFIXES lib
	PATHS
	/usr/include/
	/usr/local/include/
)
FIND_LIBRARY(BULLET_LIBRARIES_COLLISION NAMES BulletCollision
	HINTS
	$ENV{BULLETDIR}
	PATH_SUFFIXES lib
	PATHS
	/usr/include/
	/usr/local/include/
)
FIND_LIBRARY(BULLET_LIBRARIES_MATH NAMES LinearMath
	HINTS
	$ENV{BULLETDIR}
	PATH_SUFFIXES lib
	PATHS
	/usr/include/
	/usr/local/include/
)
SET(BULLET_LIBRARIES "${BULLET_LIBRARIES_SOFTBODY};${BULLET_LIBRARIES_DYNAMICS};${BULLET_LIBRARIES_COLLISION};${BULLET_LIBRARIES_MATH}")

ADD_DEFINITIONS(-DCMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

if(MINGW)
  set(CMAKE_RC_COMPILER_INIT windres)
  ENABLE_LANGUAGE(RC)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif(MINGW)

add_executable(cubit main.cpp controller.cpp map.cpp movement.cpp net.cpp ui.cpp renderer.cpp GLDebugDrawer.cpp logo/cubit.rc)

include_directories( ${SDL_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${SDLIMAGE_INCLUDE_DIR} ${FTGL_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${SQLITE_INCLUDE_DIRS} ${LZO_INCLUDE_DIR} ${BULLET_INCLUDE_DIR})
target_link_libraries (cubit ${SDL_LIBRARY} ${SDLMIXER_LIBRARY} ${SDLIMAGE_LIBRARY} ${FTGL_LIBRARY} ${FREETYPE_LIBRARY} ${OPENGL_LIBRARIES} ${Boost_LIBRARIES} ${SQLITE_LIBRARIES} ${LZO_LIBRARIES} ${BULLET_LIBRARIES})

INSTALL( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/cubit DESTINATION games)

FILE(GLOB tex "${CMAKE_CURRENT_SOURCE_DIR}/tex/*" )
INSTALL( FILES ${tex} DESTINATION share/games/cubit/tex)

FILE(GLOB soundfx "${CMAKE_CURRENT_SOURCE_DIR}/sound/fx/*" )
INSTALL( FILES ${soundfx} DESTINATION share/games/cubit/sound/fx)

FILE(GLOB soundmusic "${CMAKE_CURRENT_SOURCE_DIR}/sound/music/*" )
INSTALL( FILES ${soundmusic} DESTINATION share/games/cubit/sound/music)

INSTALL( FILES logo/cubit.svg DESTINATION share/icons/hicolor/scalable/apps)
INSTALL( FILES logo/cubit.desktop DESTINATION share/applications)
