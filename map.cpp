#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <fstream>
#include <string>

#include <stdio.h>
#include <cstdio>




#include "controller.h"

#include "map.h"

Map::Map(Controller *controller) {
	c = controller;
	
	saveArea = 0;
	loadArea = 0;
	
	harddisk = 0;
	mapGenerator = 0;
	queue_mutex = 0;
	inital_loaded = 0;
	
	dijsktra_wert = 1;
}

Map::~Map()
{
	thread_stop = 1;
	int thread_return = 0;

	if(harddisk)
		SDL_WaitThread (harddisk, &thread_return);
	
	if(queue_mutex)
		SDL_DestroyMutex(queue_mutex);
	
	iterator it;
	
	for(it = areas.begin(); it != areas.end(); it++) {
		if(storeMaps && it->second->needstore)
			store(it->second);
		delete it->second;
	}
	
	if(saveArea) sqlite3_finalize(saveArea);
	if(loadArea) sqlite3_finalize(loadArea);
}


void Map::config(const boost::program_options::variables_map& c)
{
	deleteRange = c["visualRange"].as<int>()*c["destroyAreaFaktor"].as<double>()*2;
	storeMaps = c["storeMaps"].as<bool>();
	areasPerFrameLoading = c["areasPerFrameLoading"].as<int>();
	loadRange = c["visualRange"].as<int>()*2;
}

int threaded_read_from_harddisk(void* param) {
	Map* map = (Map*)param;
	
	map->read_from_harddisk();
	
	return 0;
}

void Map::randomArea(Area* a) {
	for(int x = 0; x < AREASIZE_X; x++)
	for(int y = 0; y < AREASIZE_Y; y++)
	for(int z = 0; z < AREASIZE_Z; z++){
		int height = cos( ((2*M_PI)/180) * ((int)((a->pos.x+x)/0.7) % 180 )) * 8;
		height += sin( ((2*M_PI)/180) * ((int)((a->pos.y+y)) % 180 )) * 8;
		height += -sin( ((2*M_PI)/180) * ((int)((a->pos.x+x)/2.5) % 180 )) * 25;
		height += -cos( ((2*M_PI)/180) * ((int)((a->pos.y+y)/5) % 180 )) * 50;
		if(a->pos.z+z <  height)
			a->m[x][y][z] = 1 + (z/3) % 3;
		else
			a->m[x][y][z] = 0;
	}
}

void Map::read_from_harddisk() {
	while(!thread_stop) {
		Area* toload;
		bool empty = 1;
		do{
			SDL_LockMutex(queue_mutex);
			if(!empty) {
				loaded.push(toload);
			}
			empty = to_load.empty();
			if(!empty) {
				toload = to_load.front();
				to_load.pop();
			}
			SDL_UnlockMutex(queue_mutex);
			
			if(!empty)
				load(toload);
		} while(!empty);
		
		empty = 1;
		Area* tosave;
		
		do {
			SDL_LockMutex(queue_mutex);
/*			if(!empty) {
				saved.push(tosave);
			}*/
			empty = to_save.empty();
			if(!empty) {
				tosave = to_save.front();
				to_save.pop();
			}
			SDL_UnlockMutex(queue_mutex);
			
			if(!empty) {
				store(tosave);
				if(tosave->state == Area::STATE_DELETE)
					delete tosave;
			}
			
		} while(!empty);
		
		SDL_Delay (10);
	}
}

void Map::init()
{
	SDL_LockMutex(c->sql_mutex);
	if (sqlite3_prepare_v2(
		c->database,            /* Database handle */
		"INSERT OR REPLACE INTO area (posx, posy, posz, empty, revision, full, blocks, data) VALUES (?,?,?,?,?,?,?,?);",       /* SQL statement, UTF-8 encoded */
		-1,              /* Maximum length of zSql in bytes. */
		&saveArea,  /* OUT: Statement handle */
		0     /* OUT: Pointer to unused portion of zSql */
	) != SQLITE_OK)
		std::cout << "prepare(saveArea) hat nicht geklappt: " << sqlite3_errmsg(c->database) << std::endl;
		
	if (sqlite3_prepare_v2(
		c->database,            /* Database handle */
		"SELECT empty, revision, full, blocks, data from area where posx = ? and posy = ? and posz = ?;",       /* SQL statement, UTF-8 encoded */
		-1,              /* Maximum length of zSql in bytes. */
		&loadArea,  /* OUT: Statement handle */
		0     /* OUT: Pointer to unused portion of zSql */
	) != SQLITE_OK)
		std::cout << "prepare(loadArea) hat nicht geklappt: " << sqlite3_errmsg(c->database) << std::endl;

	SDL_UnlockMutex(c->sql_mutex);
	
	thread_stop = 0;
	queue_mutex = SDL_CreateMutex ();
	harddisk = SDL_CreateThread (threaded_read_from_harddisk,this);
}

Area* Map::getArea(BlockPosition pos)
{
	iterator it = areas.find(pos.area());
	if(it != areas.end()) {
		if(it->second->empty)
			throw AreaEmptyException();
		return it->second;
	} else {
		throw NotLoadedException(); 
	}
}

Area* Map::getOrCreate(BlockPosition pos) {
	Area** a = &areas[pos];
	if(!(*a)) {
		*a = new Area(pos);
		for(int i=0; i<DIRECTION_COUNT; i++) {
			iterator it = areas.find(pos*DIRECTION(i));
			if(it != areas.end()) {
				it->second->next[!DIRECTION(i)] = *a;
				(*a)->next[i] = it->second;
			}
		}
	}
	return *a;
}

void Map::setPosition(PlayerPosition pos)
{
	SDL_LockMutex(queue_mutex);
	
	BlockPosition p = pos.block().area();
	
	if(!inital_loaded) {
		inital_loaded = 1;
		lastpos = p;
		while(!dijsktra_queue.empty())
			dijsktra_queue.pop();
	}
	
	if(dijsktra_queue.empty()) {
		dijsktra_wert++;
		Area* a = getOrCreate(p);
		if(a->state == Area::STATE_NEW) {
			a->state == Area::STATE_LOAD;
			to_load.push(a);
		}
		dijsktra_queue.push(a);
		a->dijsktra_distance = 0;
	}
	
	for(int k=std::max(to_load.size(), to_save.size()); k<areasPerFrameLoading && !dijsktra_queue.empty(); k++) {
		Area* a = dijsktra_queue.front();
		if(a->state == Area::STATE_READY) {
			dijsktra_queue.pop();
			if(!a->full) {
				for(int i=0; i<DIRECTION_COUNT; i++) {
					Area* b = a->next[i];
					if(!b && a->dijsktra_distance < loadRange) b = getOrCreate(a->pos*DIRECTION(i));
					if(b && b->dijsktra != dijsktra_wert) {
						b->dijsktra_distance = a->dijsktra_distance+1;
						b->dijsktra = dijsktra_wert;
						dijsktra_queue.push(b);
						if(b->state == Area::STATE_NEW) {
							b->state == Area::STATE_LOAD;
							to_load.push(b);	
						}
					}
				}
			}	
		} else k = areasPerFrameLoading;
		
		if(a->dijsktra_distance > deleteRange && a->state == Area::STATE_READY) {
			a->deconfigure();
			areas_with_gllist.erase(a);
			areas.erase(a->pos);
			a->state == Area::STATE_DELETE;
			to_save.push(a);
		} else if(a->needstore && a->state == Area::STATE_READY) {
			a->needstore = 0;
			to_save.push(a);
		}
	} 
	
	while(!loaded.empty()) {
		Area* a = loaded.front();
		loaded.pop();
		
		areas[a->pos] = a;
		a->state = Area::STATE_READY;
		
		for(int i=0; i<DIRECTION_COUNT; i++)
			if(a->next[i])
				a->next[i]->needupdate = 1;
		
		if(!a->empty)
			areas_with_gllist.insert(a);
	}
	
	/*
	while(!saved.empty()) {
		Area* a = saved.front();
		saved.pop();
		
		iterator it = areas.find(a->pos);
		if(it->second->state == Area::STATE_DELETE) {
			delete a;
		}
	}
	*/
	
	SDL_UnlockMutex(queue_mutex);
}

void Map::areaLoadedIsEmpty(BlockPosition pos)
{

}

void Map::areaLoadedSuccess(Area* area)
{

}

void Map::blockChangedEvent(BlockPosition pos, Material m){

}

void Map::store(Area *a) {
	int full = a->full;
	for(int i=DIRECTION_COUNT-1; i>=0; i--) 
		full = full<<1 | a->dir_full[i];
	
	SDL_LockMutex(c->sql_mutex);
	sqlite3_bind_int(saveArea, 1, a->pos.x);
	sqlite3_bind_int(saveArea, 2, a->pos.y);
	sqlite3_bind_int(saveArea, 3, a->pos.z);
	sqlite3_bind_int(saveArea, 4, a->empty);
	sqlite3_bind_int(saveArea, 5, a->revision);
	sqlite3_bind_int(saveArea, 6, full);
	sqlite3_bind_int(saveArea, 7, a->blocks);
	if(a->empty)
		sqlite3_bind_null(saveArea, 8);
	else
		sqlite3_bind_blob(saveArea, 8, (const void*) a->m, AREASIZE_X*AREASIZE_Y*AREASIZE_Z*sizeof(Material), SQLITE_STATIC);
	
	sqlite3_step(saveArea);
	sqlite3_reset(saveArea);
	SDL_UnlockMutex(c->sql_mutex);
}

void Map::load(Area *a) {
	SDL_LockMutex(c->sql_mutex);
	sqlite3_bind_int(loadArea, 1, a->pos.x);
	sqlite3_bind_int(loadArea, 2, a->pos.y);
	sqlite3_bind_int(loadArea, 3, a->pos.z);
	if (sqlite3_step(loadArea) == SQLITE_ROW) {
		a->empty = sqlite3_column_int(loadArea, 0);
		a->revision = sqlite3_column_int(loadArea, 1);
		int full = sqlite3_column_int(loadArea, 2);
		for(int i=0; i<DIRECTION_COUNT; i++) {
			a->dir_full[i] = full & 1;
			full = full >> 1;
		}
		a->full = full;
		a->blocks = sqlite3_column_int(loadArea, 3);
		if(!a->empty)
			memcpy(a->m,sqlite3_column_blob(loadArea, 4),AREASIZE_X*AREASIZE_Y*AREASIZE_Z*sizeof(Material));
		a->state = Area::STATE_LOADED;
		sqlite3_reset(loadArea);
		SDL_UnlockMutex(c->sql_mutex);
		if(a->blocks < 0)
			recalc(a);
	} else {
		//a->state = Area::STATE_LOADED_BUT_NOT_FOUND;
		a->state = Area::STATE_LOADED;
		sqlite3_reset(loadArea);
		SDL_UnlockMutex(c->sql_mutex);
		randomArea(a);
		recalc(a);
	}
}

void Map::recalc(Area* a) {
	a->needstore = 1;
	
	a->blocks = 0;
	for(int i=0; i<DIRECTION_COUNT; i++)
		a->dir_full[i] = 1;
	
	for(int x = 0; x < AREASIZE_X; x++)
	for(int y = 0; y < AREASIZE_Y; y++)
	for(int z = 0; z < AREASIZE_Z; z++) {
		a->blocks += (a->m[x][y][z] != 0);
		for(int i=0; i<DIRECTION_COUNT; i++)
		if(!a->m[x][y][z] && (
				(x+DIRECTION_NEXT_BOX[i][0]) & ~(AREASIZE_X-1) ||
				(y+DIRECTION_NEXT_BOX[i][1]) & ~(AREASIZE_Y-1) ||
			(z+DIRECTION_NEXT_BOX[i][2]) & ~(AREASIZE_Z-1)
			)
		) a->dir_full[i] = 0;
	}
	
	a->empty = (a->blocks == 0);
	a->full = (a->blocks == AREASIZE_X*AREASIZE_Y*AREASIZE_Z);
}

Area::Area(BlockPosition p)
{
	pos = p;
	gllist_generated = 0;
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
	
	for(int x = 0; x < AREASIZE_X; x++)
	for(int y = 0; y < AREASIZE_Y; y++)
	for(int z = 0; z < AREASIZE_Z; z++)
		m[x][y][z] = 0;
	
	state = STATE_NEW;
}

Area::~Area()
{
	deconfigure();
}

