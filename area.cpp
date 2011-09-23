
#include "area.h"

#include "config.h"
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <assert.h>
#include <list>
#include <queue>

Area::Area(BlockPosition p)
{
	m = 0;
	
	pos = p;
	bullet_generated = 0;
	needupdate_gl = 1;
	needupdate_poly = 1;
	needstore = 0;
	
	empty = 1;
	full = 0;
	for(int i=0; i<DIRECTION_COUNT; i++) {
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
	polys_list = 0;
	for(int i=0; i<NUMBER_OF_LISTS; i++) {
		polys_list_size[i] = 0;
		polys_list_start[i] = 0;
	}
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
	needupdate_poly = 1;
	needstore = 1;
	
	for(int i=0; i<DIRECTION_COUNT; i++) {
		if(!operator<<(position + DIRECTION(i)) && next[i]) 
			next[i]->needupdate_poly = 1;
	}
}

bool Area::hasallneighbor() {
	if(full) return true;
	
	for(int i=0; i<DIRECTION_COUNT; i++) {
		if(!next[i]) return false;
		
		if(next[i]->state < Area::STATE_WAITING_FOR_BORDERS) return false;
	}
	
	return true;
}


void Area::recalc() {
	needupdate_poly = 1;
	blocks = 0;
	
	full = 0;
	
	if(!empty) {
		
		for(int i=0; i<AREASIZE_X*AREASIZE_Y*AREASIZE_Z; i++)
			blocks += (m[i] != 0);
		
		empty = (blocks == 0);
		full = (blocks == AREASIZE_X*AREASIZE_Y*AREASIZE_Z);
		
		if(empty) {
			delete [] m;
			m = 0;
			needstore = 1;
		}
	}
}

void Area::recalc_polys()
{
	
	needupdate_gl = 1;
	needupdate_poly = 0;
	
	// cleanup old data
	if(polys_list) delete [] polys_list;
	polys_list = 0;
	
	for(int i=0; i<NUMBER_OF_LISTS; i++) {
		polys_list_size[i] = 0;
		polys_list_start[i] = 0;
	}
	
	std::priority_queue<polygon> polys[NUMBER_OF_LISTS];

	
	
	bool emptynew = 1;
	
	int polys_count = 0;
	
	for(int dir=0; dir < DIRECTION_COUNT; dir++) {
		
		polygon array[AREASIZE_X][AREASIZE_Y][AREASIZE_Z];
		
		for(int x=pos.x; x<AREASIZE_X+pos.x; x++)
		for(int y=pos.y; y<AREASIZE_Y+pos.y; y++)
		for(int z=pos.z; z<AREASIZE_Z+pos.z; z++)  {

			BlockPosition pos = BlockPosition::create(x,y,z);
			
			polygon p;
			p.posx = x-this->pos.x;
			p.posy = y-this->pos.y;
			p.posz = z-this->pos.z;
			p.dir = dir;
			p.sizex = 1;
			p.sizey = 1;
			p.sizez = 1;
			p.m = 0;
				
			Material now = get(pos);
			if(now) { 
				emptynew = 0; 
				BlockPosition next = pos+(DIRECTION)dir;

				Material next_m;
				if((*this) << next)
					next_m = get(next);
				else if(this->next[dir] && this->next[dir]->state >= Area::STATE_WAITING_FOR_BORDERS)
					next_m = this->next[dir]->get(next);
				else 
					next_m = now;
				
				// Material 9 = water
				if(!next_m || (next_m == 9 && now != next_m )) {
					p.m = now;
				}
			}
			array[p.posx][p.posy][p.posz] = p;
		}
		
#ifdef ENABLE_POLYGON_REDUCE
		for(int x=0; x<AREASIZE_X; x++)
		for(int y=0; y<AREASIZE_Y; y++)
		for(int z=0; z<AREASIZE_Z; z++) {
			while(	array[x][y][z].m && x+array[x][y][z].sizex < AREASIZE_X &&
					array[x][y][z].m     == array[x+array[x][y][z].sizex][y][z].m &&
					array[x][y][z].sizey == array[x+array[x][y][z].sizex][y][z].sizey &&
					array[x][y][z].sizez == array[x+array[x][y][z].sizex][y][z].sizez) {
				int posx = x+array[x][y][z].sizex;
				array[x][y][z].sizex += array[posx][y][z].sizex;
				array[posx][y][z].m = 0;
			}
		}
		
		for(int x=0; x<AREASIZE_X; x++)
		for(int y=0; y<AREASIZE_Y; y++)
		for(int z=0; z<AREASIZE_Z; z++) {
			while(	array[x][y][z].m && y+array[x][y][z].sizey < AREASIZE_Y &&
					array[x][y][z].m     == array[x][y+array[x][y][z].sizey][z].m &&
					array[x][y][z].sizex == array[x][y+array[x][y][z].sizey][z].sizex &&
					array[x][y][z].sizez == array[x][y+array[x][y][z].sizey][z].sizez) {
				int posy = y+array[x][y][z].sizey;
				array[x][y][z].sizey += array[x][posy][z].sizey;
				array[x][posy][z].m = 0;
			}
		}
		
		for(int x=0; x<AREASIZE_X; x++)
		for(int y=0; y<AREASIZE_Y; y++)
		for(int z=0; z<AREASIZE_Z; z++) {
			while(	array[x][y][z].m && z+array[x][y][z].sizez < AREASIZE_Z &&
					array[x][y][z].m     == array[x][y][z+array[x][y][z].sizez].m &&
					array[x][y][z].sizex == array[x][y][z+array[x][y][z].sizez].sizex &&
					array[x][y][z].sizey == array[x][y][z+array[x][y][z].sizez].sizey) {
				int posz = z+array[x][y][z].sizez;
				array[x][y][z].sizez += array[x][y][posz].sizez;
				array[x][y][posz].m = 0;
			}
		}
#endif
		for(int x=0; x<AREASIZE_X; x++)
		for(int y=0; y<AREASIZE_Y; y++)
		for(int z=0; z<AREASIZE_Z; z++) {
			if(array[x][y][z].m) {
				if(array[x][y][z].m == 9) {
					polys[6].push(array[x][y][z]);
				} else {
					polys[dir].push(array[x][y][z]);
				}
				polys_count++;
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
				polys_list[polys_count++] = polys[l].top();
				polys[l].pop();
			}
			polys_list_size[l] = polys_count - polys_list_start[l];
		}
	}
}


