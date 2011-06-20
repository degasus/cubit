#include "area.h"
#include "config.h"

Area::Area(BlockPosition p)
{
	m = 0;
	
	pos = p;
	bullet_generated = 0;
	needupdate = 1;
	needstore = 0;
	
	empty = 1;
	full = 0;
	for(int i=0; i<DIRECTION_COUNT; i++) {
		dir_full[i] = 0;
		next[i] = 0;
	}
	
	dijsktra = 0;
	dijsktra_distance = -1;
	revision = 0;
	
	blocks = 0;
	
	state = STATE_NEW;

#ifdef USE_GLLIST
	gllist_generated = 0;
	gllist = 0;
#endif
#ifdef USE_VBO
	vbo_generated = 0;
	for(int i=0; i<NUMBER_OF_LISTS; i++) vbo[i] = 0;
#endif
	
	for(int i=0; i<NUMBER_OF_LISTS; i++) {
		vbopointer[i] = 0;
		vbo_length[i] = 0;
	}
	
	mesh = 0;
	shape = 0;
	motion = 0;
	rigid = 0;
}

Area::~Area()
{
	deconfigure();
	if(m) delete [] m;
	m = 0;
	empty = 1;
	
	assert(!mesh);
	assert(!shape);
	assert(!motion);
	assert(!rigid);
}

void Area::delete_collision(btCollisionWorld *world) {
	if(rigid) world->removeCollisionObject(rigid);
	
	if(mesh) delete mesh; mesh = 0;
	if(shape) delete shape; shape = 0;
	if(motion) delete motion; motion = 0;
	if(rigid) delete rigid; rigid = 0;
	
	bullet_generated = 0;
}

void Area::delete_opengl() {
#ifdef USE_GLLIST
	if(gllist_generated) {
		glDeleteLists(gllist,NUMBER_OF_LISTS);
	}
	gllist_generated = 0;
	gllist = 0;
#endif
#ifdef USE_VBO
	if(vbo_generated) {
		glDeleteBuffers(NUMBER_OF_LISTS,vbo);
	}
	vbo_generated = 0;
	for(int i=0; i<NUMBER_OF_LISTS; i++) vbo[i] = 0;
#endif
	
	for(int i=0; i<NUMBER_OF_LISTS; i++) {
		vbo_length[i] = 0;
		if(vbopointer[i]) {
			delete [] vbopointer[i];
			vbopointer[i] = 0;
		}
	}
}


void Area::set(BlockPosition position, Material mat) {
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