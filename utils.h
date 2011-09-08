#include <iostream>
#include <sstream>

#ifndef _UTILS_H_
#define _UTILS_H_


const int DIRECTION_COUNT = 6;
enum DIRECTION {
	DIRECTION_EAST=0,  // x positive
	DIRECTION_WEST=1,  // x negative
	DIRECTION_SOUTH=2, // y positive
	DIRECTION_NORTH=3, // y negative
	DIRECTION_UP=4,    // z positive
	DIRECTION_DOWN=5   // z negative
};


const int DIRECTION_NEXT_BOX[DIRECTION_COUNT][3] = {
	{ 1, 0, 0},
	{-1, 0, 0},
	{ 0, 1, 0},
	{ 0,-1, 0},
	{ 0, 0, 1},
	{ 0, 0,-1}
};

const int POINTS_PER_POLYGON = 4;
const double POINTS_OF_DIRECTION[DIRECTION_COUNT][POINTS_PER_POLYGON][3] = {
	{{1.0,0.0,0.0}, {1.0,0.0,1.0}, {1.0,1.0,1.0}, {1.0,1.0,0.0}},
	{{0.0,1.0,0.0}, {0.0,1.0,1.0}, {0.0,0.0,1.0}, {0.0,0.0,0.0}},
	{{1.0,1.0,0.0}, {1.0,1.0,1.0}, {0.0,1.0,1.0}, {0.0,1.0,0.0}},
	{{0.0,0.0,0.0}, {0.0,0.0,1.0}, {1.0,0.0,1.0}, {1.0,0.0,0.0}},
	{{1.0,1.0,1.0}, {1.0,0.0,1.0}, {0.0,0.0,1.0}, {0.0,1.0,1.0}},
	{{1.0,0.0,0.0}, {1.0,1.0,0.0}, {0.0,1.0,0.0}, {0.0,0.0,0.0}}
};
const double randT = 0.0;
const double TEXTUR_POSITION_OF_DIRECTION[DIRECTION_COUNT][POINTS_PER_POLYGON][2] = {
	{{1-randT,randT},{1-randT,1-randT},{randT,1-randT},{randT,randT}},
	{{1-randT,randT},{1-randT,1-randT},{randT,1-randT},{randT,randT}},
	{{1-randT,randT},{1-randT,1-randT},{randT,1-randT},{randT,randT}},
	{{1-randT,randT},{1-randT,1-randT},{randT,1-randT},{randT,randT}},
	{{1-randT,randT},{1-randT,1-randT},{randT,1-randT},{randT,randT}},
	{{1-randT,randT},{1-randT,1-randT},{randT,1-randT},{randT,randT}},
};

const double NORMAL_OF_DIRECTION[DIRECTION_COUNT][3] = {
	{ 1.0, 0.0, 0.0},
	{-1.0, 0.0, 0.0},
	{ 0.0, 1.0, 0.0},
	{ 0.0,-1.0, 0.0},
	{ 0.0, 0.0, 1.0},
	{ 0.0, 0.0,-1.0}
};


/**
 * Definiert die Position eines Blocks
 * @param x x Position (West -> Ost)
 * @param y y Position (Süd -> Nord)
 * @param z x Position (Unten -> Oben)
 */ 
struct BlockPosition {
	
	/**
	 * Will create the position at the Point (x,y,z)
	 */
	static inline BlockPosition create(int x, int y, int z) { 
		BlockPosition b; 
		b.x=x; 
		b.y=y; 
		b.z=z; 
		return b; 
	}
	
	/**
	 * @returns the next Block in this direction
	 */
	inline BlockPosition operator+(DIRECTION dir) {
		return create(
			x+DIRECTION_NEXT_BOX[dir][0],
			y+DIRECTION_NEXT_BOX[dir][1],
			z+DIRECTION_NEXT_BOX[dir][2]
 		);
	}
	
	/**
	 * @returns the next Area in this direction
	 */
	inline BlockPosition operator*(DIRECTION dir) {
		return create(
			x+DIRECTION_NEXT_BOX[dir][0]*AREASIZE_X,
			y+DIRECTION_NEXT_BOX[dir][1]*AREASIZE_Y,
			z+DIRECTION_NEXT_BOX[dir][2]*AREASIZE_Z
 		);
	}

	inline bool operator== (const BlockPosition &position) {
		return (position.x == x && position.y == y && position.z == z);
	}

	inline bool operator!= (const BlockPosition &position) {
		return !(position.x == x && position.y == y && position.z == z);
	}
	
	inline BlockPosition area() {
		return create(x & ~(AREASIZE_X-1),y & ~(AREASIZE_Y-1),z & ~(AREASIZE_Z-1));
	}

	std::string to_string() {

	std::ostringstream oss (std::ostringstream::out);

	oss << "bPos X = " << x << "; Y = " << y << "; Z = " << z;

	return oss.str();
}

	int x;
	int y;
	int z;
};


inline bool operator<(const BlockPosition &posa, const BlockPosition &posb) {
	if (posa.x<posb.x) return 1;
	if (posa.x>posb.x) return 0;
	if (posa.y<posb.y) return 1;
	if (posa.y>posb.y) return 0;
	if (posa.z<posb.z) return 1;
	return 0;
}

/**
 * return the opposite Direction
 */
inline DIRECTION operator!(const DIRECTION &dir) {
	return DIRECTION(int(dir) ^ 1);
}

#endif