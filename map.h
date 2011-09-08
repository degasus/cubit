#include <map>
#include <queue>
#include <list>
#include <cstdio>
#include <boost/program_options.hpp>
#include <SDL_opengl.h>
#include <SDL_thread.h>
#include <SDL_net.h>


#ifndef _MAP_H_
#define _MAP_H_

#include "config.h"
#include "movement.h"
#include "matrix.h"
#include "harddisk.h"
#include "utils.h"

class Map;
class Area;

class NotLoadedException {};
class AreaEmptyException {};



#include "controller.h"
#include <boost/concept_check.hpp>


#include "area.h"

class MovingObject : public btRigidBody {
public:
	MovingObject(btRigidBodyConstructionInfo body) :  btRigidBody(body) {
		m = getMotionState();
	//	setDamping(0.5,0.5);
	}
	int tex;
	btMotionState *m;
};

/**
 * Sorgt für das Laden der Karteninformation von Server
 * und stellt sie unter einfachen Funktionen bereit.
 */


class Map {
public:
	/**
	 *
	 */
	Map(Controller *controller);
	~Map();

	void init();
	
	void config(const boost::program_options::variables_map &c);
	
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
	
	void read_from_harddisk();
        void read_from_network();
	
	std::map<BlockPosition, Area*> areas;
	typedef std::map<BlockPosition, Area*>::iterator iterator;
	
	
	std::set<Area*> areas_with_gllist;
	
	std::list<MovingObject*> objects;
    
private:
	void store(Area* a);
	void load(Area* a);
	void recalc(Area* a);
	void randomArea(Area* a);
        
        void request_load_net(Area* a);

	Area* getOrCreate(BlockPosition pos);
	
	Controller *c;
	Harddisk *disk;
	
	bool storeMaps;
	int areasPerFrameLoading;
	
	// queue for loading from harddisk
	std::queue<Area*> to_load_hdd;
	std::queue<Area*> loaded_hdd;
	std::queue<Area*> to_save_hdd;
        
        // queue for loading from network
        std::queue<Area*> to_load_net;
        std::queue<Area*> loaded_net;
        std::queue<Area*> to_save_net;
        std::map<BlockPosition, Area*> load_requested_net;
        
//	std::queue<Area*> saved;
	SDL_mutex* queue_mutex;
	
	SDL_Thread* harddisk;
        SDL_Thread* network;
	SDL_Thread* mapGenerator;
	bool thread_stop;
	
	int loadRange;
	int deleteRange;
	
	BlockPosition lastpos;
	bool inital_loaded;
	bool generate_random;
	
	int dijsktra_wert;
	std::queue<Area*> dijsktra_queue;
	
	// network
	TCPsocket tcpsock;
	SDLNet_SocketSet set;
};




#endif
