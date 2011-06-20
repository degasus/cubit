#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <fstream>
#include <string>

#include <stdio.h>
#include <cstdio>


#include "config.h"
#include "controller.h"

#include "map.h"

#ifdef USE_ZLIB
#include "zlib.h"
#else
#include "lzo/lzoconf.h"
#include "lzo/lzo1x.h"
#endif


Map::Map(Controller *controller) {
	c = controller;
	
	saveArea = 0;
	loadArea = 0;
	
	harddisk = 0;
	mapGenerator = 0;
	queue_mutex = 0;
	inital_loaded = 0;
	
	dijsktra_wert = 1;
	
#ifndef USE_ZLIB
	lzo_init();
#endif
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
		
		it->second->delete_collision(c->movement->dynamicsWorld);
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
	loadRange = c["visualRange"].as<int>()*2+2;
	generate_random = c["generateRandom"].as<bool>();
}

int threaded_read_from_harddisk(void* param) {
	Map* map = (Map*)param;
	
	map->read_from_harddisk();
	
	return 0;
}

void Map::randomArea(Area* a) {
	if(a->pos.z > 92) return;
	
	a->allocm();
	a->empty = 0;
	for(int x = a->pos.x; x < AREASIZE_X+a->pos.x; x++)
	for(int y = a->pos.y; y < AREASIZE_Y+a->pos.y; y++)
	for(int z = a->pos.z; z < AREASIZE_Z+a->pos.z; z++){
		int height = cos( ((2*M_PI)/180) * ((int)((x)/0.7) % 180 )) * 8;
		height += sin( ((2*M_PI)/180) * ((int)((y)) % 180 )) * 8;
		height += -sin( ((2*M_PI)/180) * ((int)((x)/2.5) % 180 )) * 25;
		height += -cos( ((2*M_PI)/180) * ((int)((y)/5) % 180 )) * 50;
		Material m;
		
		if(z <  height - 10){
			m = 1 + ((z) % (NUMBER_OF_MATERIALS-1) + (NUMBER_OF_MATERIALS-1)) % (NUMBER_OF_MATERIALS-1);
			if(m==9) m++; // no water
		}
		else if(z <  height - 4){
			m = 1; //stone
		}
		else if(z <  height - 1 - std::rand() % 1){
			if (z >= -65)
				m = 3; //mud
			else
				m = 12; //sand
		}
		else if(z <  height){
			if(z >= 60 - std::rand() % 2)
				m = 82; //snow
			else if (z >= -65 - std::rand() % 2)
				m = 2; //grass
			else
				m = 12; //sand
		} else if(z < -70) {
			m = 9; // water
		} else {
			m = 0; //air
		}
		
		
		a->m[a->getPos(BlockPosition::create(x,y,z))] = m;
		
		assert(a->m[a->getPos(BlockPosition::create(x,y,z))] >= 0);
		assert(a->m[a->getPos(BlockPosition::create(x,y,z))] < NUMBER_OF_MATERIALS);
		
	}
}

void Map::read_from_harddisk() {
	while(!thread_stop) {
		Area* toload = 0;
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
		
		SDL_LockMutex(c->sql_mutex);
		sqlite3_exec(c->database, "BEGIN",  0,0,0);
		SDL_UnlockMutex(c->sql_mutex);
		
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
		SDL_LockMutex(c->sql_mutex);
		sqlite3_exec(c->database, "Commit",  0,0,0);
		SDL_UnlockMutex(c->sql_mutex);
		
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
		if(it->second->state != Area::STATE_READY)
			throw NotLoadedException();
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
	
	if(lastpos != p) {
		inital_loaded = 0;
		lastpos = p;
	}
	
	if(!inital_loaded) {
		// reset queue
		while(!dijsktra_queue.empty())
			dijsktra_queue.pop();
		
		// load actual position
		Area* a = getOrCreate(p);
		if(a->state == Area::STATE_NEW) {
			a->state = Area::STATE_LOAD;
			to_load.push(a);
		}
		// add it to queue
		if(a->state == Area::STATE_READY) {			
			dijsktra_queue.push(a);
			a->dijsktra_distance = 0;
			inital_loaded = 1;
			dijsktra_wert++;
			for(int d=0; d<DIRECTION_COUNT; d++)
				a->dijsktra_direction_used[d] = 0;
		}
		//std::cout << "pos: " << pos.to_string() << " " << a->state << std::endl;
	}
	//std::cout << "load: " << to_load.size() << ", store: " << to_save.size() << ", queue: " << dijsktra_queue.size() << std::endl;
	
	Area *first = 0;
	for(int k=std::max(to_load.size(), to_save.size()); k<areasPerFrameLoading && !dijsktra_queue.empty(); k++) {
		Area* a = dijsktra_queue.front();
	
		// not in a cycle
		if(first == a) {
			break;
		}
		if(!first) {
			first = a;
		}
		dijsktra_queue.pop();
		
		switch(a->state) {		
			// found, so go and try any direction
			case Area::STATE_READY:
			if(!a->full) {
				for(int i=0; i<DIRECTION_COUNT; i++) if(!a->dijsktra_direction_used[!((DIRECTION)i)]) {
					Area* b = a->next[i];
					if(!b && a->dijsktra_distance < loadRange) b = getOrCreate(a->pos*DIRECTION(i));
					if(b && (b->dijsktra != dijsktra_wert || b->dijsktra_distance > a->dijsktra_distance+1)) {
						b->dijsktra_distance = a->dijsktra_distance+1;
						b->dijsktra = dijsktra_wert;
						for(int d=0; d<DIRECTION_COUNT; d++)
							b->dijsktra_direction_used[d] = a->dijsktra_direction_used[d] || (i==d);
						dijsktra_queue.push(b);
						if(b->state == Area::STATE_NEW) {
							b->state = Area::STATE_LOAD;
							to_load.push(b);	
						}
					}
				}
			}
			break;
			
			// wait and try it again later
			case Area::STATE_LOAD:
			case Area::STATE_LOADED:
			dijsktra_queue.push(a);
			break;
			
			// not available, so stop here
			case Area::STATE_LOADED_BUT_NOT_FOUND: break;
			default: std::cout << "state: " << a->state << std::endl;
		}
		
		if(a->dijsktra_distance > deleteRange && a->state == Area::STATE_READY) {
			a->deconfigure();
			a->delete_collision(c->movement->dynamicsWorld);
			areas_with_gllist.erase(a);
			areas.erase(a->pos);
			a->state = Area::STATE_DELETE;
			to_save.push(a);
		} else if(a->needstore && a->state == Area::STATE_READY) {
			a->needstore = 0;
			to_save.push(a);
		}
	} 
	
	while(!loaded.empty()) {
		
		Area* a = loaded.front();
		loaded.pop();
		
		if(a->state == Area::STATE_LOADED){
		
			areas[a->pos] = a;
				a->state = Area::STATE_READY;
			
			for(int i=0; i<DIRECTION_COUNT; i++)
				if(a->next[i])
					a->next[i]->needupdate = 1;
			
			if(!a->empty)
				areas_with_gllist.insert(a);
			}
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
	else {
#ifdef USE_ZLIB
		// deflate
		z_stream strm;
		unsigned char out[AREASIZE];
		int out_usage;
		
		/* allocate inflate state */
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		if (deflateInit(&strm, -1) != Z_OK)
			std::cout << "fehler" << std::endl;
		
		/* decompress until deflate stream ends or end of file */
		strm.avail_in = AREASIZE;
		strm.next_in = a->m;
		strm.avail_out = AREASIZE;
		strm.next_out = out;
		
		if(deflate(&strm, Z_FINISH) != Z_STREAM_END)
			std::cout << "fehlerb" << std::endl;
		out_usage = AREASIZE-strm.avail_out;
		deflateEnd(&strm);
#else
		unsigned char wrkmem[LZO1X_1_MEM_COMPRESS];
		unsigned char out[AREASIZE + AREASIZE / 16 + 64 + 3];
		lzo_uint out_usage;
		
		int r = lzo1x_1_compress(a->m,AREASIZE,out,&out_usage,wrkmem);
#endif
		if(out_usage < AREASIZE)
			sqlite3_bind_blob(saveArea, 8, (const void*) out, out_usage, SQLITE_STATIC);
		else
			sqlite3_bind_blob(saveArea, 8, (const void*) a->m, AREASIZE, SQLITE_STATIC);
		
		
	}
	sqlite3_step(saveArea);
	sqlite3_reset(saveArea);
	SDL_UnlockMutex(c->sql_mutex); 
}

void Map::load(Area *a) {
	SDL_LockMutex(c->sql_mutex);
	sqlite3_bind_int(loadArea, 1, a->pos.x);
	sqlite3_bind_int(loadArea, 2, a->pos.y);
	sqlite3_bind_int(loadArea, 3, a->pos.z);
	int step = sqlite3_step(loadArea);
	if (step == SQLITE_ROW) {
		a->empty = sqlite3_column_int(loadArea, 0);
		a->revision = sqlite3_column_int(loadArea, 1);
		int full = sqlite3_column_int(loadArea, 2);
		for(int i=0; i<DIRECTION_COUNT; i++) {
			a->dir_full[i] = full & 1;
			full = full >> 1;
		}
		a->full = full;
		a->blocks = sqlite3_column_int(loadArea, 3);
		if(!a->empty) {
			a->allocm();
			int bytes = sqlite3_column_bytes16(loadArea, 4);
			if(bytes == AREASIZE) {
				memcpy(a->m,sqlite3_column_blob(loadArea, 4),AREASIZE);
				a->blocks = -1;
			}
			else {
#ifdef USE_ZLIB
				// inflate
				z_stream strm;
				
				/* allocate inflate state */
				strm.zalloc = Z_NULL;
				strm.zfree = Z_NULL;
				strm.opaque = Z_NULL;
				strm.avail_in = 0;
				strm.next_in = Z_NULL;
				if (inflateInit(&strm) != Z_OK)
					std::cout << "fehler" << std::endl;
				
				/* decompress until deflate stream ends or end of file */
				strm.avail_in = bytes;
				strm.next_in = (unsigned char*)sqlite3_column_blob(loadArea, 4);
				strm.avail_out = AREASIZE;
				strm.next_out = a->m;
				
				if(inflate(&strm, Z_FINISH) != Z_STREAM_END)
					std::cout << "fehlerb" << std::endl;
				inflateEnd(&strm);
#else	
				lzo_uint length = 0;
				int r = lzo1x_decompress((unsigned char*)sqlite3_column_blob(loadArea, 4), bytes, a->m,&length,0);
#endif
			}
			for (int i = 0; i < AREASIZE; i++) {
				if (a->m[i] >= NUMBER_OF_MATERIALS)
					a->m[i] = 0;	
			}
		}
		a->state = Area::STATE_LOADED;
		sqlite3_reset(loadArea);
		SDL_UnlockMutex(c->sql_mutex);
		if(a->blocks < 0)
			recalc(a);
	} else if (step == SQLITE_DONE) {
		sqlite3_reset(loadArea);
		SDL_UnlockMutex(c->sql_mutex);
		
		if(generate_random) {
			a->state = Area::STATE_LOADED;
			randomArea(a);
			recalc(a);
		} else {
			a->state = Area::STATE_LOADED_BUT_NOT_FOUND;
		}
		
	} else  {
		std::cout << "SQLITE ERROR" << std::endl;
		a->state = Area::STATE_LOADED_BUT_NOT_FOUND;
		//a->state = Area::STATE_LOADED;
		sqlite3_reset(loadArea);
		SDL_UnlockMutex(c->sql_mutex);
		//randomArea(a);
		//recalc(a);
	}
}

void Map::recalc(Area* a) {
	a->needstore = 1;
	
	a->blocks = 0;
	
	a->full = 0;
	for(int i=0; i<DIRECTION_COUNT; i++) {
		a->dir_full[i] = 0;
	}
	
	if(!a->empty) {
		for(int i=0; i<DIRECTION_COUNT; i++)
			a->dir_full[i] = 1;
		
		for(int x = 0; x < AREASIZE_X; x++)
		for(int y = 0; y < AREASIZE_Y; y++)
		for(int z = 0; z < AREASIZE_Z; z++) {
			a->blocks += (a->m[a->getPos(BlockPosition::create(x+a->pos.x,y+a->pos.y,z+a->pos.z))] != 0);
			for(int i=0; i<DIRECTION_COUNT; i++)
			if(!a->m[a->getPos(BlockPosition::create(x+a->pos.x,y+a->pos.y,z+a->pos.z))] && (
					(x+DIRECTION_NEXT_BOX[i][0]) & ~(AREASIZE_X-1) ||
					(y+DIRECTION_NEXT_BOX[i][1]) & ~(AREASIZE_Y-1) ||
				(z+DIRECTION_NEXT_BOX[i][2]) & ~(AREASIZE_Z-1)
				)
			) a->dir_full[i] = 0;
		}
		
		a->empty = (a->blocks == 0);
		a->full = (a->blocks == AREASIZE_X*AREASIZE_Y*AREASIZE_Z);
		
		if(a->empty) {
			delete [] a->m;
			a->m = 0;
		}
	} 
}
	

Material Map::getBlock(BlockPosition pos){
	iterator it = areas.find(pos.area());
	if(it == areas.end())
		throw NotLoadedException();
	
	if(it->second->state != Area::STATE_READY)
		throw NotLoadedException();
	
	return it->second->get(pos);
}


void Map::setBlock(BlockPosition pos, Material m){
	iterator it = areas.find(pos.area());
	if(it == areas.end())
		throw NotLoadedException();
	
	Area* a = it->second;
	
	if(a->state != Area::STATE_READY)
		throw NotLoadedException();

	if(a->empty && m)
		areas_with_gllist.insert(a);
	a->set(pos,m);
	
}

std::string BlockPosition::to_string() {

	std::ostringstream oss (std::ostringstream::out);

	oss << "bPos X = " << x << "; Y = " << y << "; Z = " << z;

	return oss.str();
}

