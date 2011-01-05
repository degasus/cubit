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
	
}

Map::~Map()
{
	std::map< BlockPosition, Area* >::iterator it;
	
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
	destroyArea = c["visualRange"].as<float>()*c["destroyAreaFaktor"].as<double>()*2*AREASIZE_X;
	mapDirectory = (c["workingDirectory"].as<std::string>() + "/maps/").c_str();
	storeMaps = c["storeMaps"].as<bool>();
	areasPerFrameLoading = c["areasPerFrameLoading"].as<int>();
}

void Map::init()
{
	
	if (sqlite3_prepare_v2(
		c->database,            /* Database handle */
		"INSERT OR REPLACE INTO area (posx, posy, posz, data) VALUES (?,?,?,?);",       /* SQL statement, UTF-8 encoded */
		-1,              /* Maximum length of zSql in bytes. */
		&saveArea,  /* OUT: Statement handle */
		0     /* OUT: Pointer to unused portion of zSql */
	) != SQLITE_OK)
		std::cout << "prepare(saveArea) hat nicht geklappt: " << sqlite3_errmsg(c->database) << std::endl;
		
	if (sqlite3_prepare_v2(
		c->database,            /* Database handle */
		"SELECT data from area where posx = ? and posy = ? and posz = ?;",       /* SQL statement, UTF-8 encoded */
		-1,              /* Maximum length of zSql in bytes. */
		&loadArea,  /* OUT: Statement handle */
		0     /* OUT: Pointer to unused portion of zSql */
	) != SQLITE_OK)
		std::cout << "prepare(loadArea) hat nicht geklappt: " << sqlite3_errmsg(c->database) << std::endl;
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
}

Area* Map::getArea(BlockPosition pos)
{
	if(areas.find(pos.area()) != areas.end()) {
		assert(areas[pos.area()]);
		return areas[pos.area()];
	} else {
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
			return a;
		}
		else {
			throw NotLoadedException();
		}
	}
}

void Map::setPosition(PlayerPosition pos)
{
	areasPerFrameLoadingFree = areasPerFrameLoading;
	std::map< BlockPosition, Area* >::iterator it, it_save;
	
	for(it = areas.begin(); it != areas.end(); it++) {
		if(shouldDelArea(it->second->pos,pos)) {
			it_save = it++;
			if(storeMaps)
				store(it_save->second);
			delete it_save->second;
			areas.erase(it_save);
			if(it == areas.end())
				break;
		} 
	}
}

bool Map::shouldDelArea(BlockPosition posa, PlayerPosition posp)
{
	return (
		(posa.x-posp.x)*(posa.x-posp.x) +
		(posa.y-posp.y)*(posa.y-posp.y) +
		(posa.z-posp.z)*(posa.z-posp.z)
	) >= destroyArea*destroyArea;
		
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
	sqlite3_bind_int(saveArea, 1, a->pos.x);
	sqlite3_bind_int(saveArea, 2, a->pos.y);
	sqlite3_bind_int(saveArea, 3, a->pos.z);
	sqlite3_bind_blob(saveArea, 4, (const void*) a->m, AREASIZE_X*AREASIZE_Y*AREASIZE_Z*sizeof(Material), SQLITE_STATIC);
	sqlite3_step(saveArea);
	sqlite3_reset(saveArea);
	
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
	sqlite3_bind_int(loadArea, 1, a->pos.x);
	sqlite3_bind_int(loadArea, 2, a->pos.y);
	sqlite3_bind_int(loadArea, 3, a->pos.z);
	if (sqlite3_step(loadArea) == SQLITE_ROW) {
		memcpy(a->m,sqlite3_column_blob(loadArea, 0),AREASIZE_X*AREASIZE_Y*AREASIZE_Z*sizeof(Material));
		sqlite3_reset(loadArea);
		return 1;
	} else {
		sqlite3_reset(loadArea);
		return 0;
	}
}

Area::Area(BlockPosition p)
{
	pos = p;
	gllist_generated = 0;
	needupdate = 1;
	needstore = 0;
}

Area::~Area()
{
	if(gllist_generated)
		glDeleteLists(gllist,1);
	gllist_generated = 0;
}

/*
CREATE TABLE area (
	posx INT NOT NULL, 
	posy INT NOT NULL, 
	posz INT NOT NULL, 
	empty BOOL NOT NULL DEFAULT 0,
	revision INT DEFAULT 0,
	
	data BLOB(4096),
	PRIMARY KEY (posx, posy, posz)
);
*/
