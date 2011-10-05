#include <string>
#include <queue>
#include <SDL_thread.h>
#include <SDL_net.h>

#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "config.h"
#include "utils.h"
#include <map>

class Client {
public:
	Client() {
		buffer_usage = 0;
		clientid = next_client_id++;
	}
	
	TCPsocket socket;
	char buffer[64*1024+3];
	int buffer_usage;
	int clientid;

private:
	static int next_client_id;
};

struct StructGetArea {
	StructGetArea(BlockPosition p, int r, int c) : pos(p), rev(r), client_id(c) {}
	BlockPosition pos;
	int rev;
	int client_id;
};

struct StructPushArea {
	StructPushArea(BlockPosition p, int r, char* d, int l,int cl)
	: pos(p), rev(r), length(l), client_id(cl) {
		data = new char[l];
		memcpy(data,d,l);
	}
	
	BlockPosition pos;
	int rev;
	char* data;
	int length;
	int client_id;
};

struct StructJoinArea {
	StructJoinArea(BlockPosition p, int r, int c) : pos(p), rev(r), client_id(c) {}
	BlockPosition pos;
	int rev;
	int client_id;
};

struct StructLeaveArea {
	StructLeaveArea(BlockPosition p, int c) : pos(p), client_id(c) {}
	BlockPosition pos;
	int client_id;
};

struct StructUpdateBlock {
	StructUpdateBlock(BlockPosition p, Material mm, int r, int c)
	: pos(p), m(mm), rev(r), client_id(c) {}
	BlockPosition pos;
	Material m;
	int rev;
	int client_id;
};

struct StructPlayerPosition {
	StructPlayerPosition(PlayerPosition p, int pl, int c) : pos(p), playerid(pl), client_id(c) {}
	PlayerPosition pos;
	int playerid;
	int client_id;
};

struct StructHello {
	StructHello(std::string n, int pl, int c) : name(n), playerid(pl), client_id(c) {}
	std::string name;
	int playerid;
	int client_id;
};

struct StructPlayerQuit {
	StructPlayerQuit(int pl, int c) : playerid(pl), client_id(c) {}
	int playerid;
	int client_id;
};

class Network {
public:
	// Client
	Network(std::string hostname, std::string nick, int port=PORT);
	
	// Server
	Network(int port=PORT);
	
	~Network();
	
	void send_get_area(BlockPosition pos, int revision=0, int connection=1);
	void send_push_area(BlockPosition pos, int revision, char* data, int length=AREASIZE, bool compressed=false, int connection=1);
	void send_join_area(BlockPosition pos, int revision=0, int connection=1);
	void send_leave_area(BlockPosition pos, int connection=1);
	void send_update_block(BlockPosition pos, Material m, int revision=0, int connection=1);
	void send_player_position(PlayerPosition pos, int playerid=0, int connection=1);
	void send_hello(std::string pos, int playerid=0, int connection=1);
	void send_player_quit(int playerid=0, int connection=1);
	
	///////////////////////
	
	BlockPosition recv_get_area(int *revision=0, int *connection=0);
	int recv_push_area(BlockPosition *pos, char* data, int *revision=0, int length=AREASIZE, bool compressed=false, int *connection=0);
	BlockPosition recv_join_area(int *revision=0, int *connection=0);
	BlockPosition recv_leave_area(int *connection=0);
	Material recv_update_block(BlockPosition *pos, int *revision=0, int *connection=0);
	PlayerPosition recv_player_position(int *playerid=0, int *connection=0);
	std::string recv_hello(int *playerid=0, int *connection=0);
	int recv_player_quit(int connection=0);
	
	///////////////////////
	
	bool recv_get_area_empty() { return queue_recv_get_area.empty();}
	bool recv_push_area_empty() { return queue_recv_push_area.empty();}
	bool recv_join_area_empty() { return queue_recv_join_area.empty();}
	bool recv_leave_area_empty() { return queue_recv_leave_area.empty();}
	bool recv_update_block_empty() { return queue_recv_update_block.empty();}
	bool recv_player_position_empty() { return queue_recv_player_position.empty();}
	bool recv_player_quit_empty() { return queue_recv_player_quit.empty(); };
	bool recv_hello_empty() { return queue_recv_hello.empty(); };
	
	void run();
private:
	
	// constructor
	void init();

	//connect to host
	int connect(std::string hostname, int port=PORT);
	
	// get data from client
	// if return < 0, client corrupted
    int read_client ( Client* client );
	
    void send_queues();
	
    void remove_client(Client* c);
	
	// Sockets
	bool serversocket_is_connected;
	TCPsocket server_socket;
	std::queue<Client*> client_sockets;
	std::map<int, Client*> client_map;
	SDLNet_SocketSet set_socket;
	
	// Threads & Mutexes
	SDL_Thread* thread;
	bool abort_threads;
	SDL_mutex* mutex;
	
	// Queues
	std::queue<StructGetArea> queue_recv_get_area;
	std::queue<StructPushArea> queue_recv_push_area;
	std::queue<StructJoinArea> queue_recv_join_area;
	std::queue<StructLeaveArea> queue_recv_leave_area;
	std::queue<StructUpdateBlock> queue_recv_update_block;
	std::queue<StructPlayerPosition> queue_recv_player_position;
	std::queue<StructHello> queue_recv_hello;
	std::queue<StructPlayerQuit> queue_recv_player_quit;
	
	std::queue<StructGetArea> queue_send_get_area;
	std::queue<StructPushArea> queue_send_push_area;
	std::queue<StructJoinArea> queue_send_join_area;
	std::queue<StructLeaveArea> queue_send_leave_area;
	std::queue<StructUpdateBlock> queue_send_update_block;
	std::queue<StructPlayerPosition> queue_send_player_position;
	std::queue<StructHello> queue_send_hello;
	std::queue<StructPlayerQuit> queue_send_player_quit;
	
	//Config
	std::string nick;

};

#endif