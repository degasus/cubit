#ifndef _AREA_H_
#define _AREA_H_

#include <queue>

#include <LinearMath/btAlignedObjectArray.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>

#include "config.h"
#include "utils.h"

struct polygon {
	unsigned char posx;
	unsigned char posy;
	unsigned char posz;
	
	unsigned char dir;
	Material m;
	
	unsigned char sizex;
	unsigned char sizey;
	unsigned char sizez;
};

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
	
#ifdef USE_GLLIST
	// for saving the GL-List
	uint gllist;
	bool   gllist_generated;
#endif
#ifdef USE_VBO	
	uint vbo[NUMBER_OF_LISTS];
	bool   vbo_generated;
#endif
	int    vbo_length[NUMBER_OF_LISTS];
	float* vbopointer[NUMBER_OF_LISTS];
	
	bool bullet_generated;
	bool needupdate_gl;
	bool needupdate_poly;
	bool show;
	bool needstore;
	
	Area* next[DIRECTION_COUNT];
	int dijsktra;
	int dijsktra_distance;
	bool dijsktra_direction_used[DIRECTION_COUNT];
	
	bool empty;
	bool full;
	
	int blocks;
	
	btTriangleMesh *mesh;
	btBvhTriangleMeshShape *shape;
	//btConvexTriangleMeshShape *shape;
	btDefaultMotionState *motion;
	btRigidBody *rigid;
	
	void delete_collision(btCollisionWorld *world);
	
	void delete_opengl();
	
	polygon* polys_list;
	int polys_list_size[NUMBER_OF_LISTS];
	int polys_list_start[NUMBER_OF_LISTS];
	
	enum AreaState {
		STATE_NEW,
		STATE_HDD_LOAD,
		STATE_HDD_LOADED,
		STATE_HDD_LOADED_BUT_NOT_FOUND,
		STATE_NET_LOAD,
		STATE_NET_LOADED,
		STATE_NET_LOADED_BUT_NOT_FOUND,
		STATE_WAITING_FOR_BORDERS,
		STATE_GENERATE,
		STATE_GENERATED,
		STATE_READY,
		STATE_DELETE
	} state;
	
	inline int vbo_size() {
		int s = 0;
		for(int i=0; i<NUMBER_OF_LISTS; i++)
			s+=vbo_length[i]*sizeof(float);
		return s;
	}
	
	inline int polygons_count(int l) {
		return vbo_length[l]/8/3;
	}
	
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
	
	void set(BlockPosition position, Material mat);
	
	inline std::string filename(std::string dir) {
		std::ostringstream os(std::ostringstream::out);
		os << dir << "/" << pos.x << "x" << pos.y << "x" << pos.z << ".map";
		return os.str();
	}
	
	inline void deconfigure() {
		delete_opengl();
		
		for(int i=0; i<DIRECTION_COUNT; i++) {
			if(next[i]) {
				assert(next[i]->next[!DIRECTION(i)]);
				next[i]->next[!DIRECTION(i)] = 0;
				next[i] = 0;
			}
		}
	}
	
	void recalc_polys();
	void recalc();
    bool hasallneighbor();
};

#endif