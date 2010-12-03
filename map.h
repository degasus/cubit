#ifndef _MAP_H_
#define _MAP_H_

#include <boost/program_options.hpp>
#include <map>
#include <cmath>

#include <boost/program_options.hpp>
#include <SDL/SDL_opengl.h>

#include <cstdio>
#include "movement.h"


#include "matrix.h"

class Map;
class Area;
class BlockPosition;

// including Air == 0
const int NUMBER_OF_MATERIALS = 5;

// must be a pow of two 
const int AREASIZE_X = 16;
const int AREASIZE_Y = AREASIZE_X;
const int AREASIZE_Z = AREASIZE_X;

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
	
	BlockPosition area() {
		return create(x & ~(AREASIZE_X-1),y & ~(AREASIZE_Y-1),z & ~(AREASIZE_Z-1));
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
/*
//Berechnet die Fläche, auf die von der Startposition aus (Parameter) mit der aktuellen Blickrichtung
//@return: ID der Fläche, auf die man zeigt
//Am Ende sind die Parameter auf den Schnittpunkt gesetzt
inline DIRECTION calcPointingOnInBlock(PlayerPosition pos){
	Matrix<double,3,3> left(0);
	Matrix<double,1,3> right(0);
	Matrix<double,1,3> result(0);

	//bleibt immer gleich (Blickrichtung)
	left.data[2][0] = sin(M_PI*x/180.) * cos(M_PI*y/180.);
	left.data[2][1] = -sin(M_PI*y/180.);
	left.data[2][2] = -cos(M_PI*x/180.) * cos(M_PI*y/180.);

	for(int i=0; i<DIRECTION_COUNT; i++) {
		left.data[0][0] = ;
		left.data[0][1] = ;
		left.data[0][2] = ;
		left.data[1][0] = ;
		left.data[1][1] = ;
		left.data[1][2] = ;
		right.data[0][0] = 1-*startX;
		right.data[0][1] = 1-*startY;
		right.data[0][2] = -*startZ;
		result = left.LU().solve(right);
		if( 0 <= result.data[0][0] && result.data[0][0] <= 1
		&& 0 <= result.data[0][1] && result.data[0][1] <= 1
		&& 0 < result.data[0][2]) {
			*startX = 1-result[0][0];
			*startY = 1-result[0][1];
			*startZ = 0;
			return 0;
		}
	}

	//Falls keine Austrittsebene gefunden wird Error
	return -1;
}
*/

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
        void filename();
		m[position.x-pos.x][position.y-pos.y][position.z-pos.z] = mat;
		needupdate = 1;
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
		
		return areas[pos.area()]->get(pos);
	}

	/**
	 * @param m neues Material an der Stelle (x,y,z)
	 * @throws NotLoadedException falls diese Gebiet noch nicht geladen ist
	 */
	void setBlock(BlockPosition pos, Material m){
		if(areas.find(pos.area()) == areas.end())
			throw NotLoadedException();
		
		areas[pos.area()]->set(pos,m);
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
    
private:
	bool shouldDelArea(BlockPosition posa, PlayerPosition posp);
    void store(Area *a);
    bool load(Area *a);
	
	
	Controller *c;
	Area *lastarea;
	
	std::map<BlockPosition, Area*> areas;
	
	double destroyArea;
    std::string mapDirectory;

};




#endif