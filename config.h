#ifndef _CONFIG_H_
#define _CONFIG_H_

/*
	if none is defined, drawarray is called in memory
        if both are defined, an vbo will be created and copy its data to an list. so dont use it :-)
*/
#define USE_VBO
//#define USE_GLLIST





// including Air == 0
const int NUMBER_OF_MATERIALS = 109;
const int NUMBER_OF_LISTS = 7;

// must be a pow of two 
const int AREASIZE_X = 32;
const int AREASIZE_Y = AREASIZE_X;
const int AREASIZE_Z = AREASIZE_X;

typedef unsigned char Material;

const int AREASIZE = AREASIZE_X*AREASIZE_Y*AREASIZE_Z*sizeof(Material);

#endif
