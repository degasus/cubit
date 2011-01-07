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
	lastarea = 0;
	areasPerFrameLoadingFree = 0;
	
	saveArea = 0;
	loadArea = 0;
	loadAreas = 0;
	
	harddisk = 0;
	mapGenerator = 0;
	queue_mutex = 0;
	inital_loaded = 0;
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
	if(loadAreas) sqlite3_finalize(loadAreas);
}


void Map::config(const boost::program_options::variables_map& c)
{
	destroyArea = c["visualRange"].as<int>()*c["destroyAreaFaktor"].as<double>()*2*AREASIZE_X;
	storeMaps = c["storeMaps"].as<bool>();
	areasPerFrameLoading = c["areasPerFrameLoading"].as<int>();
	visualRange = c["visualRange"].as<int>();
}

int threaded_read_from_harddisk(void* param) {
	Map* map = (Map*)param;
	
	map->read_from_harddisk();
	
	return 0;
}

int threaded_generate_new_map(void* param) {
	Map* map = (Map*)param;

	SDL_Delay (5000);
	map->generate_new_map();
	
	return 0;
}

void Map::generate_new_map()
{
	while(!thread_stop) {
		for(int x = -visualRange; x <= visualRange; x++){
			for(int y = -visualRange; y <= visualRange; y++){
				for(int z = -visualRange; z <= visualRange; z++){
					PlayerPosition curPos = c->movement->getPosition();
					BlockPosition curBlock = curPos.block();
					curBlock.x -= curBlock.x % AREASIZE_X + x*AREASIZE_X;
					curBlock.y -= curBlock.y % AREASIZE_Y + y*AREASIZE_X;
					curBlock.z -= curBlock.z % AREASIZE_Z + z*AREASIZE_X;
					std::cout << "curBlock = " << curBlock.to_string() << std::endl;
					bool loaded = true;
					try{
						getArea(curBlock);
					}
					catch(NotLoadedException e){
						std::cout << "generator NotLoadedException" << std::endl;
						loaded = false;
					}
					catch(AreaEmptyException e){

					}
					if(!loaded){
						SDL_LockMutex(c->sql_mutex);
						sqlite3_bind_int(loadArea, 1, curBlock.x);
						sqlite3_bind_int(loadArea, 2, curBlock.x);
						sqlite3_bind_int(loadArea, 3, curBlock.x);
						int res = sqlite3_step(loadArea);
						sqlite3_reset(loadArea);
						SDL_UnlockMutex(c->sql_mutex);
						if (res != SQLITE_ROW) {
							generateArea(curBlock);
						}
					}
					SDL_Delay (20);
				}
			}
		}
	}
}

void Map::generateArea(BlockPosition pos)
{
	bool empty = true;
	if(pos.z <= 0)
		empty = false;
	Area* a = new Area(pos);
	a->empty = empty;
	if(!a->empty){
		if(pos.z == 0){
			for(int x = 0; x < AREASIZE_X; x++){
				for(int y = 0; y < AREASIZE_Y; y++){
					for(int z = 0; z < AREASIZE_Z; z++){
						if(z < sin((M_PI/(x % (int)(AREASIZE_X)))* (y % (int)(AREASIZE_Y)))*AREASIZE_Z)
							a->m[x][y][z] = 0;//1+rand()%4;
						else
							a->m[x][y][z] = 0;
					}
				}
			}
		}
		if(pos.z < 0){
			for(int x = 0; x < AREASIZE_X; x++){
				for(int y = 0; y < AREASIZE_Y; y++){
					for(int z = 0; z < AREASIZE_Z; z++){
						a->m[x][y][z] = 1+rand()%4;
					}
				}
			}
		}
	}
	a->state = Area::STATE_READY;
	a->needstore = 1;
	
	std::cout << "generiere Area " << a->pos.x << "x" << a->pos.y << "x" << a->pos.z << std::endl;
	
	SDL_LockMutex(queue_mutex);
	loaded.push(a);
	SDL_UnlockMutex(queue_mutex);
}

void Map::read_from_harddisk() {
	while(!thread_stop) {
		ToLoad toload;
		bool empty = 0;
		SDL_LockMutex(queue_mutex);
		empty = to_load.empty();
		if(!empty) {
			toload = to_load.front();
			to_load.pop();
		}
		SDL_UnlockMutex(queue_mutex);
		
		if(!empty) {
			SDL_LockMutex(c->sql_mutex);
			sqlite3_bind_int(loadAreas, 1, toload.min.x);
			sqlite3_bind_int(loadAreas, 2, toload.max.x);
			sqlite3_bind_int(loadAreas, 3, toload.min.y);
			sqlite3_bind_int(loadAreas, 4, toload.max.y);
			sqlite3_bind_int(loadAreas, 5, toload.min.z);
			sqlite3_bind_int(loadAreas, 6, toload.max.z);
			
			while (sqlite3_step(loadAreas) == SQLITE_ROW) {
				Area* a = new Area(BlockPosition::create(sqlite3_column_int(loadAreas,0),
																		sqlite3_column_int(loadAreas,1),
																		sqlite3_column_int(loadAreas,2)));
				a->empty = sqlite3_column_int(loadAreas, 4);
				if(!a->empty)
				memcpy(a->m,sqlite3_column_blob(loadAreas, 3),AREASIZE_X*AREASIZE_Y*AREASIZE_Z*sizeof(Material));
				a->state = Area::STATE_READY;
				
			//	std::cout << "lade Area " << a->pos.x << "x" << a->pos.y << "x" << a->pos.z << std::endl;
				
				SDL_LockMutex(queue_mutex);
				loaded.push(a);
				SDL_UnlockMutex(queue_mutex);
			}
			sqlite3_reset(loadAreas);
			SDL_UnlockMutex(c->sql_mutex);
		}
		
		empty = 1;
		Area* tosave;
		
		do {
			SDL_LockMutex(queue_mutex);
			if(!empty) {
				saved.push(tosave);
			}
			empty = to_save.empty();
			if(!empty) {
				tosave = to_save.front();
				to_save.pop();
			}
			SDL_UnlockMutex(queue_mutex);
			
			if(!empty)
				store(tosave);
			
		} while(!empty);
		
		SDL_Delay (10);
	}
}

void Map::init()
{
	SDL_LockMutex(c->sql_mutex);
	if (sqlite3_prepare_v2(
		c->database,            /* Database handle */
		"INSERT OR REPLACE INTO area (posx, posy, posz, empty, data) VALUES (?,?,?,?,?);",       /* SQL statement, UTF-8 encoded */
		-1,              /* Maximum length of zSql in bytes. */
		&saveArea,  /* OUT: Statement handle */
		0     /* OUT: Pointer to unused portion of zSql */
	) != SQLITE_OK)
		std::cout << "prepare(saveArea) hat nicht geklappt: " << sqlite3_errmsg(c->database) << std::endl;
		
	if (sqlite3_prepare_v2(
		c->database,            /* Database handle */
		"SELECT data, empty from area where posx = ? and posy = ? and posz = ?;",       /* SQL statement, UTF-8 encoded */
		-1,              /* Maximum length of zSql in bytes. */
		&loadArea,  /* OUT: Statement handle */
		0     /* OUT: Pointer to unused portion of zSql */
	) != SQLITE_OK)
		std::cout << "prepare(loadArea) hat nicht geklappt: " << sqlite3_errmsg(c->database) << std::endl;

	if (sqlite3_prepare_v2(
		c->database,            /* Database handle */
		"SELECT posx, posy, posz, data, empty from area where posx >= ? and posx <= ? and posy >= ? and posy <= ? and posz >= ? and posz <= ?;",       /* SQL statement, UTF-8 encoded */
		-1,              /* Maximum length of zSql in bytes. */
		&loadAreas,  /* OUT: Statement handle */
		0     /* OUT: Pointer to unused portion of zSql */
	) != SQLITE_OK)
		std::cout << "prepare(loadAreas) hat nicht geklappt: " << sqlite3_errmsg(c->database) << std::endl;
		
	SDL_UnlockMutex(c->sql_mutex);
	
	thread_stop = 0;
	queue_mutex = SDL_CreateMutex ();
	harddisk = SDL_CreateThread (threaded_read_from_harddisk,this);
	mapGenerator = SDL_CreateThread (threaded_generate_new_map,this);
}


void randomArea(int schieben, Area* a) {
	int hoehe[AREASIZE_X][AREASIZE_Y];

	for(int x=0; x<AREASIZE_X; x++) for(int y=0; y<AREASIZE_Y; y++) {
		hoehe[x][y] = AREASIZE_Z/2; //-1+rand()%3;

		if(x>0 && y>0 && y<AREASIZE_Y-1) {
			hoehe[x][y] = (hoehe[x-1][y-1] + hoehe[x-1][y] + hoehe[x-1][y+1] + rand()%7 - 2) / 3;
			if(hoehe[x][y] < 1) hoehe[x][y] = 1;
			if(hoehe[x][y] > AREASIZE_Z-2) hoehe[x][y] = AREASIZE_Z-2;

		}
	}

	for(int x=0; x<AREASIZE_X; x++) for(int y=0; y<AREASIZE_Y; y++) for(int z=0; z<AREASIZE_Z; z++) {
		if(z>hoehe[x][y]-schieben)
			a->m[x][y][z] = 0;
		else
			a->m[x][y][z] = (rand()%3)+1;

	}
	a->needstore = 1;
	a->state = Area::STATE_READY;
}

Area* Map::getArea(BlockPosition pos)
{
	iterator it = areas.find(pos.area());
	if(it != areas.end()) {
		if(it->second->empty)
			throw AreaEmptyException();
		return it->second;
	} else {
		throw NotLoadedException(); /*
		if(areasPerFrameLoadingFree>0) {
			areasPerFrameLoadingFree--;
			
			Area *a = new Area(pos.area());
			areas[pos.area()] = a;
			
			for(int i=0; i<DIRECTION_COUNT; i++) {
				BlockPosition pos2 = pos.area();
				pos2.x += AREASIZE_X*DIRECTION_NEXT_BOX[i][0];
				pos2.y += AREASIZE_Y*DIRECTION_NEXT_BOX[i][1];
				pos2.z += AREASIZE_Z*DIRECTION_NEXT_BOX[i][2];
				
				if(areas.find(pos2) != areas.end()) {
					areas[pos2]->needupdate = 1;
				}
			}

			if(!storeMaps || !load(a))
				randomArea(pos.area().z, a);
			
			if(a->state != Area::STATE_READY)
				throw NotLoadedException();
			
			return a;
		}
		else {
			throw NotLoadedException();
		} */
	}
}

void Map::setPosition(PlayerPosition pos)
{
	BlockPosition p = pos.block().area();
	
	if(!inital_loaded) {
		inital_loaded = 1;
		lastpos = p;
		
		ToLoad l;
		l.min.x = p.x-AREASIZE_X*visualRange;
		l.min.y = p.y-AREASIZE_Y*visualRange;
		l.min.z = p.z-AREASIZE_Z*visualRange;
		l.max.x = p.x+AREASIZE_X*visualRange;
		l.max.y = p.y+AREASIZE_Y*visualRange;
		l.max.z = p.z+AREASIZE_Z*visualRange;
		
		SDL_LockMutex(queue_mutex);
		to_load.push(l);
		SDL_UnlockMutex(queue_mutex);
	}
	
	ToLoad l;
	l.min.x = p.x-AREASIZE_X*visualRange;
	l.min.y = p.y-AREASIZE_Y*visualRange;
	l.min.z = p.z-AREASIZE_Z*visualRange;
	l.max.x = p.x+AREASIZE_X*visualRange;
	l.max.y = p.y+AREASIZE_Y*visualRange;
	l.max.z = p.z+AREASIZE_Z*visualRange;
	bool changed = 1;
	
	SDL_LockMutex(queue_mutex);
	if(p.x > lastpos.x) {
		std::cout << "lade +x" << std::endl;
		l.min.x = p.x+AREASIZE_X*visualRange;
		l.max.x = p.x+AREASIZE_X*visualRange;
		
		for(int y=p.y-AREASIZE_Y*visualRange; y<=p.y+AREASIZE_Y*visualRange; y++)
		for(int z=p.z-AREASIZE_Z*visualRange; z<=p.z+AREASIZE_Z*visualRange; z++) {
			BlockPosition pos = BlockPosition::create(lastpos.x-AREASIZE_X*visualRange,y,z);
			iterator it = areas.find(pos);
			if(it == areas.end()) continue;
			if(it->second->needstore) {
				to_save.push(it->second);
				it->second->state = Area::STATE_TOSAVE;
			} else {
				delete it->second;
				areas_with_gllist.erase(it->second);
				areas.erase(it);
			}
		}
		lastpos.x = p.x;
	} else if(p.x < lastpos.x) {
		std::cout << "lade -x" << std::endl;
		l.min.x = p.x-AREASIZE_X*visualRange;
		l.max.x = p.x-AREASIZE_X*visualRange;
		
		for(int y=p.y-AREASIZE_Y*visualRange; y<=p.y+AREASIZE_Y*visualRange; y++)
		for(int z=p.z-AREASIZE_Z*visualRange; z<=p.z+AREASIZE_Z*visualRange; z++) {
			BlockPosition pos = BlockPosition::create(lastpos.x+AREASIZE_X*visualRange,y,z);
			iterator it = areas.find(pos);
			if(it == areas.end()) continue;
			if(it->second->needstore) {
				to_save.push(it->second);
				it->second->state = Area::STATE_TOSAVE;
			} else {
				delete it->second;
				areas_with_gllist.erase(it->second);
				areas.erase(it);
			}
		}
		lastpos.x = p.x;
	} else if(p.y > lastpos.y) {
		std::cout << "lade +y" << std::endl;
		l.min.y = p.y+AREASIZE_Y*visualRange;
		l.max.y = p.y+AREASIZE_Y*visualRange;
		
		for(int x=p.x-AREASIZE_X*visualRange; x<=p.x+AREASIZE_X*visualRange; x++)
		for(int z=p.z-AREASIZE_Z*visualRange; z<=p.z+AREASIZE_Z*visualRange; z++) {
			BlockPosition pos = BlockPosition::create(x,lastpos.y-AREASIZE_Y*visualRange,z);
			iterator it = areas.find(pos);
			if(it == areas.end()) continue;
			if(it->second->needstore) {
				to_save.push(it->second);
				it->second->state = Area::STATE_TOSAVE;
			} else {
				delete it->second;
				areas_with_gllist.erase(it->second);
				areas.erase(it);
			}
		}
		lastpos.y = p.y;
	} else if(p.y < lastpos.y) {
		std::cout << "lade -y" << std::endl;
		l.min.y = p.y-AREASIZE_Y*visualRange;
		l.max.y = p.y-AREASIZE_Y*visualRange;
		
		for(int x=p.x-AREASIZE_X*visualRange; x<=p.x+AREASIZE_X*visualRange; x++)
		for(int z=p.z-AREASIZE_Z*visualRange; z<=p.z+AREASIZE_Z*visualRange; z++) {
			BlockPosition pos = BlockPosition::create(x,lastpos.y+AREASIZE_Y*visualRange,z);
			iterator it = areas.find(pos);
			if(it == areas.end()) continue;
			if(it->second->needstore) {
				to_save.push(it->second);
				it->second->state = Area::STATE_TOSAVE;
			} else {
				delete it->second;
				areas_with_gllist.erase(it->second);
				areas.erase(it);
			}
		}
		lastpos.y = p.y;
	} else if(p.z > lastpos.z) {
		std::cout << "lade +z" << std::endl;
		l.min.z = p.z+AREASIZE_Z*visualRange;
		l.max.z = p.z+AREASIZE_Z*visualRange;
		
		for(int x=p.x-AREASIZE_X*visualRange; x<=p.x+AREASIZE_X*visualRange; x++)
		for(int y=p.y-AREASIZE_Y*visualRange; y<=p.y+AREASIZE_Y*visualRange; y++) {
			BlockPosition pos = BlockPosition::create(x,y,lastpos.z-AREASIZE_Z*visualRange);
			iterator it = areas.find(pos);
			if(it == areas.end()) continue;
			if(it->second->needstore) {
				to_save.push(it->second);
				it->second->state = Area::STATE_TOSAVE;
			} else {
				delete it->second;
				areas_with_gllist.erase(it->second);
				areas.erase(it);
			}
		}
		lastpos.z = p.z;
	} else if(p.z < lastpos.z) {
		std::cout << "lade -z" << std::endl;
		l.min.z = p.z-AREASIZE_Z*visualRange;
		l.max.z = p.z-AREASIZE_Z*visualRange;
		
		for(int x=p.x-AREASIZE_X*visualRange; x<=p.x+AREASIZE_X*visualRange; x++)
		for(int y=p.y-AREASIZE_Y*visualRange; y<=p.y+AREASIZE_Y*visualRange; y++) {
			BlockPosition pos = BlockPosition::create(x,y,lastpos.z+AREASIZE_Z*visualRange);
			iterator it = areas.find(pos);
			if(it == areas.end()) continue;
			if(it->second->needstore) {
				to_save.push(it->second);
				it->second->state = Area::STATE_TOSAVE;
			} else {
				delete it->second;
				areas_with_gllist.erase(it->second);
				areas.erase(it);
			}
		}
		lastpos.z = p.z;
	} else changed = 0;
	
	if(changed) {
		std::cout << l.min.x << "x" <<l.min.y << "x" <<l.min.z << " - " <<l.max.x << "x" <<l.max.y << "x" <<l.max.z << std::endl;
		to_load.push(l);
	}
	
	while(!loaded.empty()) {
		Area* a = loaded.front();
		loaded.pop();
		
		areas[a->pos] = a;
		areas_with_gllist.insert(a);
	}
	
	while(!saved.empty()) {
		Area* a = saved.front();
		saved.pop();
		
		iterator it = areas.find(a->pos);
		if(it->second->state == Area::STATE_TOSAVE) {
			areas.erase(it);
			delete a;
			areas_with_gllist.erase(a);
		}
	}
	
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
	/*
	std::ofstream of(a->filename(mapDirectory).c_str(),std::ofstream::binary);
	
	of.write((char*) a->m, sizeof(Material)*AREASIZE_X*AREASIZE_Y*AREASIZE_Z);
	
	of.close();
	*/
	//std::cout << "store " << a->pos.x <<"x"<<a->pos.y <<"x"<<a->pos.z <<std::endl;
	
	SDL_LockMutex(c->sql_mutex);
	sqlite3_bind_int(saveArea, 1, a->pos.x);
	sqlite3_bind_int(saveArea, 2, a->pos.y);
	sqlite3_bind_int(saveArea, 3, a->pos.z);
	sqlite3_bind_int(saveArea, 4, a->empty);
	if(a->empty)
		sqlite3_bind_null(saveArea, 5);
	else
		sqlite3_bind_blob(saveArea, 5, (const void*) a->m, AREASIZE_X*AREASIZE_Y*AREASIZE_Z*sizeof(Material), SQLITE_STATIC);
	sqlite3_step(saveArea);
	sqlite3_reset(saveArea);
	SDL_UnlockMutex(c->sql_mutex);
}

bool Map::load(Area *a) {
	/*
	std::ifstream i(a->filename(mapDirectory).c_str(),std::ifstream::binary);
	bool success = 0;
	if (i.is_open()) {
		i.read((char*) a->m, sizeof(Material)*AREASIZE_X*AREASIZE_Y*AREASIZE_Z);
		success = 1;
	}
	i.close();
	return success;
	*/
	/*
	SDL_LockMutex(c->sql_mutex);
	sqlite3_bind_int(loadArea, 1, a->pos.x);
	sqlite3_bind_int(loadArea, 2, a->pos.y);
	sqlite3_bind_int(loadArea, 3, a->pos.z);
	if (sqlite3_step(loadArea) == SQLITE_ROW) {
		memcpy(a->m,sqlite3_column_blob(loadArea, 0),AREASIZE_X*AREASIZE_Y*AREASIZE_Z*sizeof(Material));
		sqlite3_reset(loadArea);
		a->state = Area::STATE_READY;
		return 1;
	} else {
		sqlite3_reset(loadArea);
		return 0;
	}
	SDL_UnlockMutex(c->sql_mutex);
	*/
}

Area::Area(BlockPosition p)
{
	pos = p;
	gllist_generated = 0;
	needupdate = 1;
	needstore = 0;
	
	empty = 1;
	
	state = STATE_NEW;
}

Area::~Area()
{
	if(gllist_generated)
		glDeleteLists(gllist,1);
	gllist_generated = 0;
}

