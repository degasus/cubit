#include "SDL_net.h"
#include <set>
#include <map>

#include "config.h"
#include "harddisk.h"
#include "network.h"
#include "utils.h"

struct Player {
	Player() {}
	Player(std::string nick, int pl) : nick(nick), playerid(pl) {}
	std::string nick;
	int playerid;
};

class Server {
public:
	Server();
	~Server();
	
	void run();
	
private:
	Harddisk *harddisk;
	Network *network;
	bool stop;
	
	std::map<int, Player> clients;
	
	std::map<BlockPosition, std::set<int> > joined_clients;
	
	int randomArea(BlockPosition bPos, char* buffer);
};

