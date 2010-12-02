#include <iostream>

#include "controller.h"

#include "map.h"

Map::Map(Controller *controller) {
	c = controller;
	lastarea = 0;
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
		if(z>hoehe[x][y]+schieben)
			a->m[x][y][z] = 0;
		else
			a->m[x][y][z] = (rand()%(NUMBER_OF_MATERIALS-1))+1;

	}
}
Area* Map::getArea(BlockPosition pos)
{
	Area *a = &areas[pos.x & ~(AREASIZE_X-1)][pos.y & ~(AREASIZE_Y-1)][pos.z & ~(AREASIZE_Z-1)];
	if(a->isnew) {
		randomArea(pos.z & ~(AREASIZE_Z-1), a);
		a->isnew = 0;
		a->pos.x = pos.x & ~(AREASIZE_X-1);
		a->pos.y = pos.y & ~(AREASIZE_Y-1);
		a->pos.z = pos.z & ~(AREASIZE_Z-1);
		
	}
	return a;
}

Material Map::getBlock(BlockPosition pos)
{
	Area *a = &areas[pos.x & ~(AREASIZE_X-1)][pos.y & ~(AREASIZE_Y-1)][pos.z & ~(AREASIZE_Z-1)];
	return a->m[(pos.x+AREASIZE_X)%AREASIZE_X][(pos.y+AREASIZE_Y)%AREASIZE_Y][(pos.z+AREASIZE_Z)%AREASIZE_Z];
}

void Map::setBlock(BlockPosition pos, Material m)
{
	Area *a = &areas[pos.x & ~(AREASIZE_X-1)][pos.y & ~(AREASIZE_Y-1)][pos.z & ~(AREASIZE_Z-1)];
	a->m[(pos.x+AREASIZE_X)%AREASIZE_X][(pos.y+AREASIZE_Y)%AREASIZE_Y][(pos.z+AREASIZE_Z)%AREASIZE_Z] = m;
}

void Map::setPosition(PlayerPosition pos)
{

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
}



