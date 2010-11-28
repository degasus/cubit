#ifndef _MAP_H_
#define _MAP_H_

class Map;
class Area;

const int MATERIALS = 4;
const int AREASIZE_X = 32;
const int AREASIZE_Y = 32;
const int AREASIZE_Z = 32;

typedef unsigned char Material;
class NotLoadedException {};
class EmptyException {};





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
	 * @param x x Position (West -> Ost)
	 * @param y y Position (Süd -> Nord)
	 * @param z x Position (Unten -> Oben)
	 * @returns Material an der Stelle (x,y,z)
	 * @throws NotLoadedException
	 */
	Material getMaterial(int x, int y, int z);

	/**
	 *
	 * @param x x Position (West -> Ost)
	 * @param y y Position (Süd -> Nord)
	 * @param z x Position (Unten -> Oben)
	 * @param m neues Material an der Stelle (x,y,z)
	 * @throws NotLoadedException falls diese Gebiet noch nicht geladen ist
	 */
	void setMaterial(int x, int y, int z, Material m);

	/**
	 * @param x x Position (West -> Ost)
	 * @param y y Position (Süd -> Nord)
	 * @param z x Position (Unten -> Oben)
	 * @returns Area an der Stelle (x,y,z)
	 * @throws NotLoadedException falls diese Gebiet noch nicht geladen ist
	 * @throws EmptyException falls dieses Gebiet nur Luft beinhaltet
	 */
	Area* getArea(int x, int y, int z);

	/**
	 * Setzt die aktuelle Position des Spielers.
	 * Dies wird benötigt, um zu erkennen, welche
	 * Gebiete geladen werden müssen.
	 * @param x x Position (West -> Ost)
	 * @param y y Position (Süd -> Nord)
	 * @param z x Position (Unten -> Oben)
	 */
	void setPosition(int x, int y, int z);
private:

};

/**
 * kleines Gebiet auf der Karte.
 * Dies ist ein einzelner Abschnitt beim Rendern
 * und beim Laden übers Netz.
 */
class Area {
public:
	Material m[AREASIZE_X][AREASIZE_Y][AREASIZE_Z];
};

#endif