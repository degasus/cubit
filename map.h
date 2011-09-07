#include <map>
#include <queue>
#include <list>
#include <cstdio>
#include <boost/program_options.hpp>
#include <SDL_opengl.h>
#include <SDL_thread.h>


#ifndef _MAP_H_
#define _MAP_H_

#include "config.h"
#include "movement.h"
#include "matrix.h"

class Map;
class Area;
class BlockPosition;

// including Air == 0
const int NUMBER_OF_MATERIALS = 109;

// must be a pow of two 
const int AREASIZE_X = 32;
const int AREASIZE_Y = AREASIZE_X;
const int AREASIZE_Z = AREASIZE_X;

typedef unsigned char Material;

const int AREASIZE = AREASIZE_X*AREASIZE_Y*AREASIZE_Z*sizeof(Material);

class NotLoadedException {};
class AreaEmptyException {};

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

#include "controller.h"
#include <boost/concept_check.hpp>

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

	std::string to_string();

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

#include "area.h"

class MovingObject : public btRigidBody {
public:
	MovingObject(btRigidBodyConstructionInfo body) :  btRigidBody(body) {
		m = getMotionState();
	//	setDamping(0.5,0.5);
	}
	int tex;
	btMotionState *m;
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
	~Map();

	void init();
	
	void config(const boost::program_options::variables_map &c);
	
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
	
	void read_from_harddisk();
	
	std::map<BlockPosition, Area*> areas;
	typedef std::map<BlockPosition, Area*>::iterator iterator;
	
	
	std::set<Area*> areas_with_gllist;
	
	std::list<MovingObject*> objects;
    
private:
	void store(Area* a);
	void load(Area* a);
	void recalc(Area* a);
	void randomArea(Area* a);

	Area* getOrCreate(BlockPosition pos);
	
	Controller *c;
	
	bool storeMaps;
	int areasPerFrameLoading;
	
	// prepared statements
	sqlite3_stmt *saveArea;
	sqlite3_stmt *loadArea;
	
	// queue for loading from harddisk
	std::queue<Area*> to_load;
	std::queue<Area*> loaded;
	std::queue<Area*> to_save;
//	std::queue<Area*> saved;
	SDL_mutex* queue_mutex;
	
	SDL_Thread* harddisk;
	SDL_Thread* mapGenerator;
	bool thread_stop;
	
	int loadRange;
	int deleteRange;
	
	BlockPosition lastpos;
	bool inital_loaded;
	bool generate_random;
	
	int dijsktra_wert;
	std::queue<Area*> dijsktra_queue;
	
};




#endif
