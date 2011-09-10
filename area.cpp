
#include "area.h"

#include "config.h"
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>

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
	
	polys_list = 0;
	for(int i=0; i<NUMBER_OF_LISTS; i++) {
		polys_list_size[i] = 0;
		polys_list_start[i] = 0;
	}
}

Area::~Area()
{
	deconfigure();
	if(m) delete [] m;
	m = 0;
	empty = 1;
	
	if(polys_list) delete [] polys_list;
	for(int i=0; i<NUMBER_OF_LISTS; i++) {
		polys_list_size[i] = 0;
		polys_list_start[i] = 0;
	}
	assert(!mesh);
	assert(!shape);
	assert(!motion);
	assert(!rigid);
}

void Area::recalc_polys()
{
	// cleanup old data
	if(polys_list) delete [] polys_list;
	for(int i=0; i<NUMBER_OF_LISTS; i++) {
		polys_list_size[i] = 0;
		polys_list_start[i] = 0;
	}
	
	std::queue<polygon> polys[NUMBER_OF_LISTS];

	bool emptynew = 1;
	
	int polys_count = 0;
	
	for(int x=pos.x; x<AREASIZE_X+pos.x; x++)
	for(int y=pos.y; y<AREASIZE_Y+pos.y; y++)
	for(int z=pos.z; z<AREASIZE_Z+pos.z; z++)  {

		BlockPosition pos = BlockPosition::create(x,y,z);

		Material now = get(pos);
		if(now) { 
			emptynew = 0;
			for(int dir=0; dir < DIRECTION_COUNT; dir++) /*if(!area->dijsktra_direction_used[dir])*/{
				BlockPosition next = pos+(DIRECTION)dir;

				Material next_m;
				if((*this) << next)
					next_m = get(next);
				else if(this->next[dir] && this->next[dir]->state == Area::STATE_READY)
					next_m = this->next[dir]->get(next);
				else 
					next_m = now;
				
				// Material 9 = water
				if(!next_m || (next_m == 9 && now != next_m )) {
					polygon p;
					p.posx = x-this->pos.x;
					p.posy = y-this->pos.y;
					p.posz = z-this->pos.z;
					p.sizex = 1;
					p.sizey = 1;
					p.dir = dir;
					p.m = now;
					
					//std::cout << int(p.posx) << " " << int(p.posy) << " " << int(p.posz) << std::endl;
					
					if(now == 9) {
						polys[6].push(p);
					} else {
						polys[dir].push(p);
					}
					polys_count++;
				}
			}
		}
	}
	if(emptynew) {
		needstore = 1;
		empty = 1;
	}
	if(polys_count) {
		polys_list = new polygon[polys_count];
		polys_count = 0;
		
		for(int l=0; l<NUMBER_OF_LISTS; l++) {
			polys_list_start[l] = polys_count;
			while(!polys[l].empty()) {
				polys_list[polys_count++] = polys[l].front();
				polys[l].pop();
			}
			polys_list_size[l] = polys_count - polys_list_start[l];
		}
	}
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