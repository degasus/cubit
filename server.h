#include "SDL_net.h"
#include "config.h"
#include "harddisk.h"
#include "network.h"
#include "utils.h"

class Server {
public:
	Server();
	~Server();
	
	void run();
	
private:
	Harddisk *harddisk;
	Network *network;
	bool stop;
	
	int randomArea(BlockPosition bPos, char* buffer);
};

