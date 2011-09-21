#include <map>
#include <queue>
#include <list>
#include <cstdio>
#include <boost/program_options.hpp>
#include <SDL_thread.h>
#include <SDL_net.h>


#ifndef _MAP_H_
#define _MAP_H_

#include "config.h"
#include "movement.h"
#include "matrix.h"
#include "network.h"
#include "harddisk.h"
#include "utils.h"

class Map;
class Area;

class NotLoadedException {};
class AreaEmptyException {};



#include "controller.h"
#include <boost/concept_check.hpp>


#include "area.h"

#ifdef ENABLE_OBJETS
class MovingObject : public btRigidBody {
public:
	MovingObject(btRigidBodyConstructionInfo body) :  btRigidBody(body) {
		m = getMotionState();
	//	setDamping(0.5,0.5);
	}
	int tex;
	btMotionState *m;
};
#endif

struct OtherPlayer {
	OtherPlayer() : pos(PlayerPosition::create(0., 0., 0., 0., 0.)) {}
	OtherPlayer(PlayerPosition p, std::string n) : pos(p), name(n) {}
	PlayerPosition pos;
	std::string name;
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
	
	void read_from_harddisk();
	void generator();
	
	std::string debug_msg();
	
	std::map<BlockPosition, Area*> areas;
	typedef std::map<BlockPosition, Area*>::iterator iterator;
	
	
	std::set<Area*> areas_with_gllist;
	
#ifdef ENABLE_OBJETS
	std::list<MovingObject*> objects;
#endif
	
	std::map<int, OtherPlayer> otherPlayers;
    
//private:
	void store(Area* a);
	void load(Area* a);
        
	void request_load_net(Area* a);

	Area* getOrCreate(BlockPosition pos);
	
	Controller *c;
	Harddisk *disk;
	Network *network;
	
	bool storeMaps;
	int areasPerFrameLoading;
	
	// queue for loading from harddisk
	std::queue<Area*> to_load_hdd;
	std::queue<Area*> loaded_hdd;
	std::queue<Area*> to_save_hdd;
	std::list<Area*> to_generate;
	std::queue<Area*> generated;
	
        
	// queue for loading from network
	std::map<BlockPosition, Area*> load_requested_net;
        
//	std::queue<Area*> saved;
	SDL_mutex* queue_mutex;
	
	SDL_Thread* harddisk;
	SDL_Thread** mapGenerator;
	int mapGenerator_counts;
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
