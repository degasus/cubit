#include <iostream>
#include <fstream>
#include <string>

#include <cstdio>




#include "controller.h"

#include "map.h"

Map::Map(Controller *controller) {
	c = controller;
	lastarea = 0;
}

Map::~Map()
{
	std::map< BlockPosition, Area* >::iterator it;
	
	for(it = areas.begin(); it != areas.end(); it++) {
		store(it->second);
		delete it->second;
	}
}


void Map::config(const boost::program_options::variables_map& c)
{
	destroyArea = c["destroyArea"].as<double>();
	mapDirectory = c["mapDirectory"].as<std::string>();
}

void Map::init()
{

}


void randomArea(int schieben, Area* a) {
	int hoehe[AREASIZE_X][AREASIZE_Y];

	for(int x=0; x<AREASIZE_X; x++) for(int y=0; y<AREASIZE_Y; y++) {
		hoehe[x][y] = AREASIZE_Z/2;

		if(x>0 && y>0 && y<AREASIZE_Y-1) {
			hoehe[x][y] = (hoehe[x-1][y-1] + hoehe[x-1][y] + hoehe[x-1][y+1] + rand()%8 - 2) / 3;
			if(hoehe[x][y] < 1) hoehe[x][y] = 1;
			if(hoehe[x][y] > AREASIZE_Z-2) hoehe[x][y] = AREASIZE_Z-2;

		}
	}

	for(int x=0; x<AREASIZE_X; x++) for(int y=0; y<AREASIZE_Y; y++) for(int z=0; z<AREASIZE_Z; z++) {
		if(z>hoehe[x][y]-schieben)
			a->m[x][y][z] = 0;
		else
			a->m[x][y][z] = (rand()%(NUMBER_OF_MATERIALS-1))+1;

	}
}
Area* Map::getArea(BlockPosition pos)
{
	Area **a = &areas[pos.area()];
	if(!(*a)) {
		(*a) = new Area(pos.area());
		if(!load(*a))
			randomArea(pos.area().z, (*a));	
	}
	return (*a);
}

void Map::setPosition(PlayerPosition pos)
{
	std::map< BlockPosition, Area* >::iterator it, it_save;
	
	for(it = areas.begin(); it != areas.end(); it++) {
		if(shouldDelArea(it->second->pos,pos)) {
			it_save = it++;
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

void Map::blockChangedEvent(BlockPosition pos, Material m)
{

}

void Map::store(Area *a) {
	std::ofstream of(a->filename(mapDirectory).c_str(),std::ofstream::binary);
	
	of.write((char*) a->m, sizeof(Material)*AREASIZE_X*AREASIZE_Y*AREASIZE_Z);
	
	of.close();
}

bool Map::load(Area *a) {
	std::ifstream i(a->filename(mapDirectory).c_str(),std::ifstream::binary);
	bool success = 0;
	if (i.is_open()) {
		i.read((char*) a->m, sizeof(Material)*AREASIZE_X*AREASIZE_Y*AREASIZE_Z);
		success = 1;
	}
	i.close();
	return success;
}

Area::Area(BlockPosition p)
{
	pos = p;
	gllist_generated = 0;
}

Area::~Area()
{
	if(gllist_generated)
		glDeleteLists(gllist,1);
	gllist_generated = 0;
}


