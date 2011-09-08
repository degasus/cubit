cmake_minimum_required (VERSION 2.6)
project (cubit)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

find_package( SDL REQUIRED )
find_package( SDL_mixer REQUIRED )
find_package( SDL_image REQUIRED )
find_package( SDL_net REQUIRED )
find_package( Freetype REQUIRED )
find_package( OpenGL REQUIRED )
find_package( Boost COMPONENTS program_options filesystem system )

# local modules
find_package( ZLIB REQUIRED )
find_package( Sqlite3 REQUIRED )
find_package( Bullet REQUIRED )
find_package( FTGL REQUIRED )



ADD_DEFINITIONS(-DCMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
ADD_DEFINITIONS(-DGL_GLEXT_PROTOTYPES)

if(MINGW)
  set(CMAKE_RC_COMPILER_INIT windres)
  ENABLE_LANGUAGE(RC)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif(MINGW)

add_executable(cubit area.cpp main.cpp controller.cpp map.cpp movement.cpp ui.cpp renderer.cpp harddisk.cpp GLDebugDrawer.cpp logo/cubit.rc)
add_executable(cubit_converter converter.cpp harddisk.cpp)
add_executable(cubit_server server.cpp harddisk.cpp)

include_directories( ${SDL_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${SDLIMAGE_INCLUDE_DIR} ${SDLNET_INCLUDE_DIR} ${FTGL_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${SQLITE_INCLUDE_DIRS} ${BULLET_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})
target_link_libraries (cubit ${SDL_LIBRARY} ${SDLMIXER_LIBRARY} ${SDLIMAGE_LIBRARY} ${SDLNET_LIBRARY} ${FTGL_LIBRARY} ${FREETYPE_LIBRARY} ${OPENGL_LIBRARIES} ${Boost_LIBRARIES} ${SQLITE3_LIBRARIES} ${BULLET_LIBRARIES} ${ZLIB_LIBRARIES})
target_link_libraries (cubit_converter ${SDL_LIBRARY} ${Boost_LIBRARIES} ${SQLITE3_LIBRARIES} ${ZLIB_LIBRARIES})
target_link_libraries (cubit_server ${SDL_LIBRARY} ${SDLNET_LIBRARY} ${Boost_LIBRARIES} ${SQLITE3_LIBRARIES} ${ZLIB_LIBRARIES})

INSTALL( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/cubit DESTINATION games)
INSTALL( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/cubit_converter DESTINATION games)
INSTALL( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/cubit_server DESTINATION games)

FILE(GLOB fonts "${CMAKE_CURRENT_SOURCE_DIR}/fonts/*" )
INSTALL( FILES ${fonts} DESTINATION share/games/cubit/fonts)

FILE(GLOB tex "${CMAKE_CURRENT_SOURCE_DIR}/tex/*" )
INSTALL( FILES ${tex} DESTINATION share/games/cubit/tex)

FILE(GLOB soundfx "${CMAKE_CURRENT_SOURCE_DIR}/sound/fx/*" )
INSTALL( FILES ${soundfx} DESTINATION share/games/cubit/sound/fx)

FILE(GLOB soundmusic "${CMAKE_CURRENT_SOURCE_DIR}/sound/music/*" )
INSTALL( FILES ${soundmusic} DESTINATION share/games/cubit/sound/music)

INSTALL( FILES logo/cubit.svg DESTINATION share/icons/hicolor/scalable/apps)
INSTALL( FILES logo/cubit.desktop DESTINATION share/applications)

# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set (CPACK_PACKAGE_VERSION_MAJOR "0")
set (CPACK_PACKAGE_VERSION_MINOR "0")
set (CPACK_PACKAGE_VERSION_PATCH "4")
set (CPACK_DEBIAN_PACKAGE_MAINTAINER "markus@selfnet.de")
include (CPack)
