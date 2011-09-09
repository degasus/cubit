#ifndef _CONFIG_H_
#define _CONFIG_H_

/*
	if none is defined, drawarray is called in memory
        if both are defined, an vbo will be created and copy its data to an list. so dont use it :-)
*/
#ifndef __WIN32__

#define USE_VBO
//#define USE_GLLIST

#else

//#define USE_VBO
#define USE_GLLIST

#endif


enum Commands {
  GET_AREA = 1,
  PUSH_AREA = 2,
  JOIN_AREA = 3,
  LEAVE_AREA = 4,
  UPDATE_BLOCK = 5,
  PLAYER_POSITION = 6,
};

// including Air == 0
const int NUMBER_OF_MATERIALS = 109;
const int NUMBER_OF_LISTS = 7;

// must be a pow of two 
const int AREASIZE_X = 32;
const int AREASIZE_Y = AREASIZE_X;
const int AREASIZE_Z = AREASIZE_X;

typedef unsigned char Material;

const int AREASIZE = AREASIZE_X*AREASIZE_Y*AREASIZE_Z*sizeof(Material);


const int PORT = 1337;
const int MAXCLIENTS = 256;

#endif
