#ifndef _NET_H_
#define _NET_H_

class Net;
class AreaChangedEvent;

#include "map.h"

/**
 * This class manages the network connection between the clients and the server.
 */
class Net {
public:
	// log into the given server
	Net(char* server, void* profile);

	// log off
	~Net();

	void loadArea(BlockPosition block);
	void subscribeToArea(BlockPosition block);
	void unsubscribeFromArea(BlockPosition block);

	void changeBlock(BlockPosition block, Material newBlockType);
	void moveTo(PlayerPosition pos);


private:

};

class AreaChangedEvent {
public:
	// what type of event occured in this area
	enum eventType {
		BLOCK_CHANGED,
		PLAYER_MOVED
	} ;

	union unio {

		// eventType == BLOCK_CHANGED
		struct blo {
			// position of the block that changed
			BlockPosition block;

			// new material for this block
			Material newBlockType;
		} block;

		// eventType == PLAYER_MOVED
		struct play {
			// user ID of the player who moved to a new position
			unsigned char uid;

			// new position in this area
			float posX;
			float posY;
		} player;
	} uni;// union

};

#endif
