#include <map>
#include <queue>
#include <list>
#include <cstdio>

#include <boost/program_options.hpp>
#include <SDL_opengl.h>
#include <SDL_thread.h>

#ifndef _MAP_H_
#define _MAP_H_

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

/**
 * kleines Gebiet auf der Karte.
 * Dies ist ein einzelner Abschnitt beim Rendern
 * und beim Laden übers Netz.
 */
class Area {
public:
	Area(BlockPosition p);
	~Area();
	
	BlockPosition pos;
	Material* m;
	
	// compairable with the server
	int revision;
	
	// for saving the GL-List
	GLuint gllist;
	bool gllist_generated;
	bool needupdate;
	
	bool needstore;
	
	Area* next[DIRECTION_COUNT];
	int dijsktra;
	int dijsktra_distance;
	
	bool empty;
	bool full;
	bool dir_full[DIRECTION_COUNT];
	
	int blocks;
	
	btTriangleMesh *mesh;
	btBvhTriangleMeshShape *shape;
	btDefaultMotionState *motion;
	btRigidBody *rigid;
	
	inline void delete_collision(btCollisionWorld *world) {
		if(rigid) world->removeCollisionObject(rigid);
		
		if(mesh) delete mesh; mesh = 0;
		if(shape) delete shape; shape = 0;
		if(motion) delete motion; motion = 0;
		if(rigid) delete rigid; rigid = 0;
	}
	
	enum AreaState {
		STATE_NEW,
		STATE_LOAD,
		STATE_LOADED,
		STATE_LOADED_BUT_NOT_FOUND,
		STATE_READY,
		STATE_DELETE
	} state;
	
	inline void allocm() {
		m = new Material[AREASIZE_X*AREASIZE_Y*AREASIZE_Z];
		for(int i=0; i<AREASIZE_X*AREASIZE_Y*AREASIZE_Z; i++)
			m[i] = 0;
	}
	
	inline int getPos(BlockPosition position) {
		int p = (position.x-pos.x)*AREASIZE_Y*AREASIZE_Z + (position.y-pos.y)*AREASIZE_Z + (position.z-pos.z);
		assert(p >= 0 && p < AREASIZE_X*AREASIZE_Y*AREASIZE_Z);
		return p;
	}
	
	/**
	 * calculate if the position is in this area
	 * @returns true, if the position is in this area
	 */
	inline bool operator<< (const BlockPosition &position) {
		return
			(position.x & ~(AREASIZE_X-1))  == pos.x && 
			(position.y & ~(AREASIZE_Y-1))  == pos.y &&
			(position.z & ~(AREASIZE_Z-1))  == pos.z
		;
	}
	
	/**
	 * fetch the Material at an absolute position
	 */
	inline Material get(BlockPosition position) {
		if(empty) return 0;
		
		assert(operator<<(position));		
		assert(m[getPos(position)] >= 0 && m[getPos(position)] < NUMBER_OF_MATERIALS);
		
		return m[getPos(position)];
	}
	
	inline void set(BlockPosition position, Material mat) {
		assert(operator<<(position));
		assert(mat >= 0 && mat < NUMBER_OF_MATERIALS);
		
		if(empty && mat) {
			allocm();
			empty = 0;
			blocks = 1;
			m[getPos(position)] = mat;
		} else if(full && !mat) {
			full = 0;
			blocks = AREASIZE_X*AREASIZE_Y*AREASIZE_Z-1;
			m[getPos(position)] = mat;
		} else if(!full && !empty) {
			Material oldmat = m[getPos(position)];
			if(mat && !oldmat) blocks++;
			if(!mat && oldmat) blocks--;

			m[getPos(position)] = mat;
			
			if(blocks == AREASIZE_X*AREASIZE_Y*AREASIZE_Z)
				full = 1;
			else if(blocks == 0) {
				empty = 1;
				delete [] m;
				m = 0;
			}			
		}
		needupdate = 1;
		needstore = 1;
		
		for(int i=0; i<DIRECTION_COUNT; i++) {
			if(!operator<<(position + DIRECTION(i)) && next[i]) 
				next[i]->needupdate = 1;
		}
	}
	
	inline std::string filename(std::string dir) {
		std::ostringstream os(std::ostringstream::out);
		os << dir << "/" << pos.x << "x" << pos.y << "x" << pos.z << ".map";
		return os.str();
	}
	
	inline void deconfigure() {
		if(gllist_generated)
			glDeleteLists(gllist,1);
		gllist_generated = 0;
		gllist = 0;
		
	//	if(colShape) delete colShape;
	//	colShape = 0;
		
		for(int i=0; i<DIRECTION_COUNT; i++) {
			if(next[i]) {
				assert(next[i]->next[!DIRECTION(i)]);
				next[i]->next[!DIRECTION(i)] = 0;
				next[i] = 0;
			}
		}
	}
	
	/*
	inline void check() {
		if(!empty) {
			assert(m);
			for(int i=0; i<AREASIZE_X*AREASIZE_Y*AREASIZE_Z; i++) {
				assert(m[i]>=0 && m[i] < NUMBER_OF_MATERIALS);
			}
		} else assert(!m);
	}
	*/
};

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
	
	
	int dijsktra_wert;
	std::queue<Area*> dijsktra_queue;
	
};




#endif
