#include "controller.h"

#include "map.h"

Map::Map(Controller *controller) {
	c = controller;
	
	area.pos.x = 0;
	area.pos.y = 0;
	area.pos.z = 0;
	
	
	int hoehe[AREASIZE_X][AREASIZE_Y];

	for(int x=0; x<AREASIZE_X; x++) for(int y=0; y<AREASIZE_Y; y++) {
		hoehe[x][y] = AREASIZE_Z/2;

		if(x>0 && y>0 && y<AREASIZE_Y-1) {
			hoehe[x][y] = (hoehe[x-1][y-1] + hoehe[x-1][y] + hoehe[x-1][y+1] + rand()%4 - 2) / 3;
			if(hoehe[x][y] < 1) hoehe[x][y] = 1;
			if(hoehe[x][y] > AREASIZE_Z-2) hoehe[x][y] = AREASIZE_Z-2;

		}
	}

	for(int x=0; x<AREASIZE_X; x++) for(int y=0; y<AREASIZE_Y; y++) for(int z=0; z<AREASIZE_Z; z++) {
		if(z>hoehe[x][y])
			area.m[x][y][z] = 0;
		else
			area.m[x][y][z] = (rand()%(NUMBER_OF_MATERIALS-1))+1;

	}
}

Area* Map::getArea(BlockPosition pos)
{
	return &area;
}

Material Map::getBlock(BlockPosition pos)
{
	return area.m[(pos.x+AREASIZE_X)%AREASIZE_X][(pos.y+AREASIZE_Y)%AREASIZE_Y][(pos.z+AREASIZE_Z)%AREASIZE_Z];
}

void Map::setBlock(BlockPosition pos, Material m)
{
	area.m[(pos.x+AREASIZE_X)%AREASIZE_X][(pos.y+AREASIZE_Y)%AREASIZE_Y][(pos.z+AREASIZE_Z)%AREASIZE_Z] = m;
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
}






