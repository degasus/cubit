#ifndef _MAP_H_
#define _MAP_H_

#include <boost/program_options.hpp>
#include <SDL/SDL_opengl.h>

class Map;
class Area;
class BlockPosition;

// including Air == 0
const int NUMBER_OF_MATERIALS = 5;

const int AREASIZE_X = 16;
const int AREASIZE_Y = 16;
const int AREASIZE_Z = 16;

typedef unsigned char Material;
class NotLoadedException {};
class AreaEmptyException {};

const int DIRECTION_COUNT = 6;
enum DIRECTION {
	DIRECTION_EAST=0,
	DIRECTION_WEST=1,
	DIRECTION_SOUTH=2,
	DIRECTION_NORTH=3,
	DIRECTION_UP=4,
	DIRECTION_DOWN=5
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

const double TEXTUR_POSITION_OF_DIRECTION[DIRECTION_COUNT][POINTS_PER_POLYGON][2] = {
	{{1.0,0.0},{1.0,1.0},{0.0,1.0},{0.0,0.0}},
	{{1.0,0.0},{1.0,1.0},{0.0,1.0},{0.0,0.0}},
	{{1.0,0.0},{1.0,1.0},{0.0,1.0},{0.0,0.0}},
	{{1.0,0.0},{1.0,1.0},{0.0,1.0},{0.0,0.0}},
	{{1.0,0.0},{1.0,1.0},{0.0,1.0},{0.0,0.0}},
	{{1.0,0.0},{1.0,1.0},{0.0,1.0},{0.0,0.0}}
	
};

const double NORMAL_OF_DIRECTION[DIRECTION_COUNT][3] = {
	{ 1.0, 0.0, 0.0},
	{-1.0, 0.0, 0.0},
	{ 0.0, 1.0, 0.0},
	{ 0.0,-1.0, 0.0},
	{ 0.0, 0.0, 1.0},
	{ 0.0, 0.0,-1.0}
};

#include "controller.h"

/**
 * Definiert die Position eines Blocks
 * @param x x Position (West -> Ost)
 * @param y y Position (Süd -> Nord)
 * @param z x Position (Unten -> Oben)
 */ 
struct BlockPosition {
	
	static inline BlockPosition create(int x, int y, int z) { 
		BlockPosition b; 
		b.x=x; 
		b.y=y; 
		b.z=z; 
		return b; 
	}
	
	BlockPosition operator+(DIRECTION dir) {
		return create(
			x+DIRECTION_NEXT_BOX[dir][0],
			y+DIRECTION_NEXT_BOX[dir][1],
			z+DIRECTION_NEXT_BOX[dir][2]
 		);
	}
	
	int x;
	int y;
	int z;
};

/**
 * kleines Gebiet auf der Karte.
 * Dies ist ein einzelner Abschnitt beim Rendern
 * und beim Laden übers Netz.
 */
class Area {
public:
	Area();
	BlockPosition pos;
	Material m[AREASIZE_X][AREASIZE_Y][AREASIZE_Z];
	int revision;
	bool gllist_generated;
	GLuint gllist;
	bool needupdate;
};

/**
 * Sorgt für das Laden der Karteninformation von Server
 * und stellt sie unter einfachen Funktionen bereit.
 */
class Map {
public:
	/**
	 *
	 */
	Map(Controller *controller);

	/**
	 * @returns Material an der Stelle (x,y,z)
	 * @throws NotLoadedException
	 */
	Material getBlock(BlockPosition pos);

	/**
	 * @param m neues Material an der Stelle (x,y,z)
	 * @throws NotLoadedException falls diese Gebiet noch nicht geladen ist
	 */
	void setBlock(BlockPosition pos, Material m);

	/**
	 * @returns Area an der Stelle (x,y,z)
	 * @throws NotLoadedException falls diese Gebiet noch nicht geladen ist
	 * @throws AreaEmptyException falls dieses Gebiet nur Luft beinhaltet
	 */
	Area* getArea(BlockPosition pos);

	/**
	 * Setzt die aktuelle Position des Spielers.
	 * Dies wird benötigt, um zu erkennen, welche
	 * Gebiete geladen werden müssen.
	 */
	void setPosition(PlayerPosition pos);

	/**
	 * only callable from net
	 */
	void areaLoadedSuccess(Area* area);

	/**
	 * only callable from net
	 */
	void areaLoadedIsEmpty(BlockPosition pos);

	/**
	 * only callable from net
	 */
	void blockChangedEvent(BlockPosition pos, Material m);
private:
	Area area;
	Controller *c;
};




#endif