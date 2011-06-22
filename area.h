#ifndef _AREA_H_
#define _AREA_H_

class Area;

#include "controller.h"

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
	GLuint gllist;
	bool   gllist_generated;
#endif
#ifdef USE_VBO	
	GLuint vbo[NUMBER_OF_LISTS];
	bool   vbo_generated;
#endif
	int    vbo_length[NUMBER_OF_LISTS];
	GLfloat* vbopointer[NUMBER_OF_LISTS];
	
	bool bullet_generated;
	bool needupdate;
	bool show;
	bool needstore;
	
	Area* next[DIRECTION_COUNT];
	int dijsktra;
	int dijsktra_distance;
	bool dijsktra_direction_used[DIRECTION_COUNT];
	
	bool empty;
	bool full;
	bool dir_full[DIRECTION_COUNT];
	
	int blocks;
	
	btTriangleMesh *mesh;
	btBvhTriangleMeshShape *shape;
	//btConvexTriangleMeshShape *shape;
	btDefaultMotionState *motion;
	btRigidBody *rigid;
	
	void delete_collision(btCollisionWorld *world);
	
	void delete_opengl();
	
	enum AreaState {
		STATE_NEW,
		STATE_LOAD,
		STATE_LOADED,
		STATE_LOADED_BUT_NOT_FOUND,
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
};

#endif