#ifndef _MAP_H_
#define _MAP_H_

class Map;
class Area;
class BlockPosition;

const int MATERIALS = 4;
const int AREASIZE_X = 32;
const int AREASIZE_Y = 32;
const int AREASIZE_Z = 32;

typedef unsigned char Material;
class NotLoadedException {};
class AreaEmptyException {};

#include "movement.h"

/**
 * Sorgt für das Laden der Karteninformation von Server
 * und stellt sie unter einfachen Funktionen bereit.
 */
class Map {
public:
	/**
	 *
	 */
	Map();

	/**
	 * @returns Material an der Stelle (x,y,z)
	 * @throws NotLoadedException
	 */
	Material getBlock(BlockPosition pos);

	/**
	 * @param m neues Material an der Stelle (x,y,z)
	 * @throws NotLoadedException falls diese Gebiet noch nicht geladen ist
	 */
	void setBlock(BlockPosition pos, Material m);

	/**
	 * @returns Area an der Stelle (x,y,z)
	 * @throws NotLoadedException falls diese Gebiet noch nicht geladen ist
	 * @throws AreaEmptyException falls dieses Gebiet nur Luft beinhaltet
	 */
	Area* getArea(BlockPosition pos);

	/**
	 * Setzt die aktuelle Position des Spielers.
	 * Dies wird benötigt, um zu erkennen, welche
	 * Gebiete geladen werden müssen.
	 */
	void setPosition(PlayerPosition pos);

	/**
	 * only callable from net
	 */
	void areaLoadedSuccess(Area* area);

	/**
	 * only callable from net
	 */
	void areaLoadedIsEmpty(BlockPosition pos);

	/**
	 * only callable from net
	 */
	void blockChangedEvent(BlockPosition pos, Material m);
private:

};

/**
	 * Definiert die Position eines Blocks
	 * @param x x Position (West -> Ost)
	 * @param y y Position (Süd -> Nord)
	 * @param z x Position (Unten -> Oben)
	 */ 
class BlockPosition {	
	int x;
	int y;
	int z;
};

/**
 * kleines Gebiet auf der Karte.
 * Dies ist ein einzelner Abschnitt beim Rendern
 * und beim Laden übers Netz.
 */
class Area {
public:
	BlockPosition pos;
	Material m[AREASIZE_X][AREASIZE_Y][AREASIZE_Z];
	int revision;
};

#endif