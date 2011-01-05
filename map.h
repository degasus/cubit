#include <boost/program_options.hpp>
#include <map>
#include <cmath>
#include <queue>
#include <cstdio>

#include <boost/program_options.hpp>
#include <SDL/SDL_opengl.h>
#include <SDL/SDL_thread.h>

#ifndef _MAP_H_
#define _MAP_H_

#include "movement.h"


#include "matrix.h"

class Map;
class Area;
class BlockPosition;

// including Air == 0
const int NUMBER_OF_MATERIALS = 81;

// must be a pow of two 
const int AREASIZE_X = 16;
const int AREASIZE_Y = AREASIZE_X;
const int AREASIZE_Z = AREASIZE_X;

typedef unsigned char Material;
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
	BlockPosition operator+(DIRECTION dir) {
		return create(
			x+DIRECTION_NEXT_BOX[dir][0],
			y+DIRECTION_NEXT_BOX[dir][1],
			z+DIRECTION_NEXT_BOX[dir][2]
 		);
	}

	inline bool operator== (const BlockPosition &position) {
		return (position.x == x && position.y == y && position.z == z);
	}

	inline bool operator!= (const BlockPosition &position) {
		return !(position.x == x && position.y == y && position.z == z);
	}
	
	BlockPosition area() {
		return create(x & ~(AREASIZE_X-1),y & ~(AREASIZE_Y-1),z & ~(AREASIZE_Z-1));
	}

	inline std::string to_string(){

		std::ostringstream oss (std::ostringstream::out);

		oss << "bPos X = " << x << "; Y = " << y << "; Z = " << z;

		return oss.str();
	}

	int x;
	int y;
	int z;
};

//bool operator<(const BlockPosition &posa, const BlockPosition &posb);

inline bool operator<(const BlockPosition &posa, const BlockPosition &posb) {
	if (posa.x<posb.x) return 1;
	if (posa.x>posb.x) return 0;
	if (posa.y<posb.y) return 1;
	if (posa.y>posb.y) return 0;
	if (posa.z<posb.z) return 1;
	return 0;
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
	Material m[AREASIZE_X][AREASIZE_Y][AREASIZE_Z];
	
	// compairable with the server
	int revision;
	
	// for saving the GL-List
	GLuint gllist;
	bool gllist_generated;
	bool needupdate;
	
	bool needstore;
	
	enum AreaState {
		STATE_NEW,
		STATE_READY
	} state;
	
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
		return m[position.x-pos.x][position.y-pos.y][position.z-pos.z];
	}
	inline void set(BlockPosition position, Material mat) {
		m[position.x-pos.x][position.y-pos.y][position.z-pos.z] = mat;
		needupdate = 1;
		needstore = 1;
	}
	inline std::string filename(std::string dir) {
		std::ostringstream os(std::ostringstream::out);
		os << dir << "/" << pos.x << "x" << pos.y << "x" << pos.z << ".map";
		return os.str();
	}
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
	inline Material getBlock(BlockPosition pos){
		if(areas.find(pos.area()) == areas.end())
			throw NotLoadedException();
		
		Area* a = areas[pos.area()];
		if(a->state != Area::STATE_READY)
			throw NotLoadedException();
		
		return a->get(pos);
	}

	/**
	 * @param m neues Material an der Stelle (x,y,z)
	 * @throws NotLoadedException falls diese Gebiet noch nicht geladen ist
	 */
	void setBlock(BlockPosition pos, Material m){
		if(areas.find(pos.area()) == areas.end())
			throw NotLoadedException();
		
		Area* a = areas[pos.area()];
		
		if(a->state != Area::STATE_READY)
			throw NotLoadedException();
		
		a->set(pos,m);
		
		for(int i=0; i<DIRECTION_COUNT; i++) {
			if(areas.find((pos+(DIRECTION)i).area()) != areas.end())
				areas[(pos+(DIRECTION)i).area()]->needupdate = 1;
		}
	}

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
	
    
private:
	bool shouldDelArea(BlockPosition posa, PlayerPosition posp);
	void store(Area *a);
	bool load(Area *a);
	
	
	Controller *c;
	Area *lastarea;
	
	
	double destroyArea;
	bool storeMaps;
	int areasPerFrameLoading;
	int areasPerFrameLoadingFree;
	
	// prepared statements
	sqlite3_stmt *saveArea;
	sqlite3_stmt *loadArea;
	sqlite3_stmt *loadAreas;
	
	// queue for loading from harddisk
	struct ToLoad {BlockPosition min; BlockPosition max; };
	std::queue<ToLoad> to_load;
	std::queue<Area*> loaded;
	SDL_mutex* queue_mutex;
	
	SDL_Thread* harddisk;
	bool thread_stop;
	
	int visualRange;
	BlockPosition lastpos;
	bool inital_loaded;
	
};




#endif
