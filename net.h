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
	Net(char* server, void* profile, void() callback);

	// log off
	~Net();

	void loadArea(BlockPos block);
	void subscribeToArea(BlockPos block);
	void unsubscribeFromArea(BlockPos block);

	void changeBlock(BlockPos block, Material newBlockType);
	void moveTo(PlayerPosition pos, Vector3D viewDirection);


private:

};

class AreaChangedEvent {
public:
	// what type of event occured in this area
	enum {
		BLOCK_CHANGED,
		PLAYER_MOVED
	} eventType;

	union {

		// eventType == BLOCK_CHANGED
		struct {
			// position of the block that changed
			BlockPos block;

			// new material for this block
			Material newBlockType;
		} block;

		// eventType == PLAYER_MOVED
		struct {
			// user ID of the player who moved to a new position
			unsigned char uid;

			// new position in this area
			float posX;
			float posY;
		} player;
	} // union

};

#endif
