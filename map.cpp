#include <iostream>

#include "controller.h"

#include "map.h"

Map::Map(Controller *controller) {
	c = controller;
	lastarea = 0;
}

void Map::config(const boost::program_options::variables_map& c)
{
	destroyArea = c["destroyArea"].as<double>();
}

void Map::init()
{

}


void randomArea(int schieben, Area* a) {
	a->pos.x = 0;
	a->pos.y = 0;
	a->pos.z = 0;
	
	
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
	Area *a = &areas[pos.area()];
	if(a->isnew) {
		randomArea(pos.z & ~(AREASIZE_Z-1), a);
		a->isnew = 0;
		a->pos = pos.area();		
	}
	return a;
}

void Map::setPosition(PlayerPosition pos)
{
	std::map< BlockPosition, Area >::iterator it, it_save;
	
	for(it = areas.begin(); it != areas.end(); it++) {
		if(shouldDelArea(it->second.pos,pos)) {
			it_save = it++;
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

Area::Area()
{
	gllist_generated = 0;
	isnew = 1;
}

Area::~Area()
{
	if(gllist_generated)
		glDeleteLists(gllist,1);
	gllist_generated = 0;
}


