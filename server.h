#include "SDL_net.h"
#include <set>
#include <map>

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
	
	std::map<BlockPosition, std::set<int> > joined_clients;
	
	int randomArea(BlockPosition bPos, char* buffer);
};

