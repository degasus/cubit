#include <zlib.h>
#include <signal.h>

#include "network.h"

int Client::next_client_id = 1;

// Client
Network::Network ( std::string hostname, int port ) {
	serversocket_is_connected = 0;
	Client *client = new Client();
	IPaddress ip;
	
	if(SDLNet_ResolveHost(&ip,hostname.c_str(),port)==-1) {
		printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
		exit(1);
	}

	client->socket=SDLNet_TCP_Open(&ip);
	if(!client->socket) {
		printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
		exit(2);
	}

	set_socket=SDLNet_AllocSocketSet(16);
	if(!set_socket) {
		printf("SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
		exit(1); //most of the time this is a major error, but do what you want.
	}

	if(SDLNet_TCP_AddSocket(set_socket,client->socket)==-1) {
		printf("SDLNet_AddSocket: %s\n", SDLNet_GetError());
		// perhaps you need to restart the set and make it bigger...
	}
	
	client_sockets.push(client);
	client_map[client->clientid] = client;
	init();
}

// Host
Network::Network ( int port ) {
	serversocket_is_connected = 1;
	IPaddress ip;
	
	if(SDLNet_ResolveHost(&ip,0,port)==-1) {
		printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
		exit(1);
	}

	server_socket=SDLNet_TCP_Open(&ip);
	if(!server_socket) {
		printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
		exit(2);
	}
	
	set_socket=SDLNet_AllocSocketSet(MAXCLIENTS);
	if(!set_socket) {
		printf("SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
		exit(1); //most of the time this is a major error, but do what you want.
	}

	if(SDLNet_TCP_AddSocket(set_socket,server_socket)==-1) {
		printf("SDLNet_AddSocket: %s\n", SDLNet_GetError());
		// perhaps you need to restart the set and make it bigger...
	}
	init();
}

Network::~Network() {
	abort_threads = 1;
	int thread_return;
	SDL_WaitThread(thread, &thread_return);
	
	SDL_DestroyMutex(mutex);
	while(!client_sockets.empty())
		remove_client(client_sockets.front());
	
	if(serversocket_is_connected) {
		SDLNet_TCP_DelSocket(set_socket, server_socket);
		SDLNet_TCP_Close(server_socket);
	}
	SDLNet_FreeSocketSet(set_socket);
}


int start_network(void *n) {
	Network *net = (Network*)n;
	net->run();
	return 0;
}

void Network::init() {
#ifndef __WIN32__
	signal(SIGPIPE, SIG_IGN);
#endif
	mutex = SDL_CreateMutex();
	abort_threads = 0;
	thread = SDL_CreateThread(start_network, this);
}

void Network::run () {
	while(!abort_threads) {
		int numready = SDLNet_CheckSockets(set_socket, 10);
		if(numready==-1) {
			printf("SDLNet_CheckSockets: %s\n", SDLNet_GetError());
			//most of the time this is a system error, where perror might help you.
			perror("SDLNet_CheckSockets");
		} else if(numready>0) {
			
			// checking for new connections
			while(serversocket_is_connected && SDLNet_SocketReady(server_socket)) {
				Client* client = new Client();
				client->socket=SDLNet_TCP_Accept(server_socket);
				if(client->socket) {
					client_sockets.push(client);
					client_map[client->clientid] = client;
					IPaddress* remote_ip = SDLNet_TCP_GetPeerAddress(client->socket);
					if(!remote_ip) {
						printf("SDLNet_TCP_GetPeerAddress: %s\n", SDLNet_GetError());
						printf("This may be a server socket.\n");
					}
					printf("Added client %d.%d.%d.%d:%d to list!\n",remote_ip->host%256,remote_ip->host/(256)%256,remote_ip->host/(256*256)%256,remote_ip->host/(256*256*256),remote_ip->port);
					if(SDLNet_TCP_AddSocket(set_socket,client->socket)==-1) {
						printf("SDLNet_AddSocket: %s\n", SDLNet_GetError());
						// perhaps you need to restart the set and make it bigger...
						
						// TODO: say to the client, that the server is full
					}
				} else {
					delete client;
				}
			}
			
			// checking all clients
			for(int i=0; i<client_sockets.size() && !client_sockets.empty(); i++) {
				Client *client = client_sockets.front();
				client_sockets.pop();
				
				// if read_client return <0, then an error has happend, so delete it
				bool toremove = 0;
				while(SDLNet_SocketReady(client->socket) && !toremove) {
					toremove = read_client(client) < 0;
				}
				if(toremove) {
					remove_client(client);
				} else {
					client_sockets.push(client);
				}
			}
		}
		send_queues();
	}
}

int Network::read_client ( Client* c ) {
	int maxlen;
	if(c->buffer_usage < 3)
		maxlen = 3 - c->buffer_usage;
	else
		maxlen = 3 + SDLNet_Read16(c->buffer+1) - c->buffer_usage;
          
	//printf("want to recv %d bytes\n", maxlen); 
	int result = SDLNet_TCP_Recv(c->socket,c->buffer+c->buffer_usage,maxlen);
	if(result<=0) {
		// An error may have occured, but sometimes you can just ignore it
		// It may be good to disconnect socket because it is likely invalid now.
		
		//FIXME: Errorhandling, reconnect, close socket, ...
		return -1;
	} else {
		c->buffer_usage += result;
		//printf("%d recv, %d availible, %d wanted\n", result, client->buffer_usage, 3 + SDLNet_Read16(client->buffer+1));
		if(c->buffer_usage >= 3 && c->buffer_usage == 3 + SDLNet_Read16(c->buffer+1)) {
			int posx, posy, posz, rev, playerid, len;
			Material material;
			double pposx, pposy, pposz, pposh, pposv;
			
			SDL_LockMutex(mutex);
			
			BlockPosition bPos;
			PlayerPosition pPos;
			
			//printf("Data recv: %d bytes, %d type, %s\n", client->buffer_usage ,client->buffer[0],client->buffer+3);
			switch ((Commands)c->buffer[0]) {
			case GET_AREA:
				posx = SDLNet_Read32(c->buffer+3);
				posy = SDLNet_Read32(c->buffer+7);
				posz = SDLNet_Read32(c->buffer+11);
				rev  = SDLNet_Read32(c->buffer+15);
				bPos = BlockPosition::create(posx, posy, posz).area();
				
				printf("GET_AREA: posx=%d, posy=%d, posz=%d, revision=%d\n", posx, posy, posz, rev);
				
				queue_recv_get_area.push(StructGetArea(bPos, rev, c->clientid));
			break;
			case PUSH_AREA:
				posx = SDLNet_Read32(c->buffer+3);
				posy = SDLNet_Read32(c->buffer+7);
				posz = SDLNet_Read32(c->buffer+11);
				rev  = SDLNet_Read32(c->buffer+15);
				len  = SDLNet_Read16(c->buffer+1);
				bPos = BlockPosition::create(posx, posy, posz).area();
				
				printf("PUSH_AREA: posx=%d, posy=%d, posz=%d, revision=%d, len(data)=%d\n", posx, posy, posz, rev, len);
				
				queue_recv_push_area.push(StructPushArea(bPos, rev, c->buffer+19, len, c->clientid));
			break;
			case JOIN_AREA:
				posx = SDLNet_Read32(c->buffer+3);
				posy = SDLNet_Read32(c->buffer+7);
				posz = SDLNet_Read32(c->buffer+11);
				rev  = SDLNet_Read32(c->buffer+15);
				bPos = BlockPosition::create(posx, posy, posz).area();
				printf("JOIN_AREA: posx=%d, posy=%d, posz=%d, revision=%d\n", posx, posy, posz, rev);
				
				queue_recv_join_area.push(StructJoinArea(bPos, rev, c->clientid));
			break;
			case LEAVE_AREA:
				posx = SDLNet_Read32(c->buffer+3);
				posy = SDLNet_Read32(c->buffer+7);
				posz = SDLNet_Read32(c->buffer+11);
				bPos = BlockPosition::create(posx, posy, posz).area();
				printf("LEAVE_AREA: posx=%d, posy=%d, posz=%d\n", posx, posy, posz);
				queue_recv_leave_area.push(StructLeaveArea(bPos, c->clientid));
			break;
			case UPDATE_BLOCK:
				posx = SDLNet_Read32(c->buffer+3);
				posy = SDLNet_Read32(c->buffer+7);
				posz = SDLNet_Read32(c->buffer+11);
				material = c->buffer[15];
				rev  = SDLNet_Read32(c->buffer+16);
				bPos = BlockPosition::create(posx, posy, posz);
				printf("UPDATE_BLOCK: posx=%d, posy=%d, posz=%d, matrial=%d, revision=%d\n", posx, posy, posz, material, rev);
				queue_recv_update_block.push(StructUpdateBlock(bPos, material, rev, c->clientid));
				
			break;
			case PLAYER_POSITION:
				playerid = SDLNet_Read32(c->buffer+3);
				pposx = ((double*)(c->buffer+7))[0];
				pposy = ((double*)(c->buffer+7))[1];
				pposz = ((double*)(c->buffer+7))[2];
				pposh = ((double*)(c->buffer+7))[3];
				pposv = ((double*)(c->buffer+7))[4];
				printf("PLAYER_POSITION: playerid=%d, posx=%f, posy=%f, posz=%f, posh=%f, posv=%f\n", playerid, pposx, pposy, pposz, pposh, pposv);
				pPos = PlayerPosition::create(pposx, pposy, pposz, pposh, pposv);
				queue_recv_player_position.push(StructPlayerPosition(pPos, playerid, c->clientid));
			break;
			default:
				printf("UNKNOWN COMMAND\n");
			break;
			}
			c->buffer_usage = 0;
			
			SDL_UnlockMutex(mutex);
		}
	}
	return 0;
}

void Network::remove_client(Client* client) {
	client_map.erase(client->clientid);
	if(SDLNet_TCP_DelSocket(set_socket,client->socket)==-1) {
		printf("SDLNet_DelSocket: %s\n", SDLNet_GetError());
		// perhaps the socket is not in the set
	}
	printf("Client disconnected\n");    
	SDLNet_TCP_Close(client->socket);
	delete client;
	
	for(int i=0; i<client_sockets.size() && !client_sockets.empty(); i++) {
		Client *c = client_sockets.front();
		client_sockets.pop();
		if(c != client)
			client_sockets.push(c);
	}
}


void Network::send_queues() {
	SDL_LockMutex(mutex);
	
	char buffer[64*1024+3];
	std::map<int, Client*>::iterator it;
	
	buffer[0] = (char)GET_AREA;
	SDLNet_Write16(16,buffer+1);
	while(!queue_send_get_area.empty()) {
		StructGetArea s = queue_send_get_area.front();
		SDLNet_Write32(s.pos.x, buffer+3);
		SDLNet_Write32(s.pos.y, buffer+7);
		SDLNet_Write32(s.pos.z, buffer+11);
		SDLNet_Write32(s.rev,   buffer+15);
		if((it = client_map.find(s.client_id)) != client_map.end() &&
			SDLNet_TCP_Send(it->second->socket, buffer, 19) != 19) 
		{
			remove_client(it->second);
		}
		queue_send_get_area.pop();
	}
	
	buffer[0] = (char)PUSH_AREA;
	while(!queue_send_push_area.empty()) {
		StructPushArea s = queue_send_push_area.front();
		SDLNet_Write16(s.length+16,buffer+1);
		SDLNet_Write32(s.pos.x, buffer+3);
		SDLNet_Write32(s.pos.y, buffer+7);
		SDLNet_Write32(s.pos.z, buffer+11);
		SDLNet_Write32(s.rev,   buffer+15);
		memcpy(buffer+19,s.data, s.length);
		delete [] s.data;
		if((it = client_map.find(s.client_id)) != client_map.end() &&
			SDLNet_TCP_Send(it->second->socket, buffer, 19+s.length) != 19+s.length) 
		{
			remove_client(it->second);
		}
		queue_send_push_area.pop();
	}
	
	buffer[0] = (char)JOIN_AREA;
	SDLNet_Write16(16,buffer+1);
	while(!queue_send_join_area.empty()) {
		StructJoinArea s = queue_send_join_area.front();
		SDLNet_Write32(s.pos.x, buffer+3);
		SDLNet_Write32(s.pos.y, buffer+7);
		SDLNet_Write32(s.pos.z, buffer+11);
		SDLNet_Write32(s.rev,   buffer+15);
		if((it = client_map.find(s.client_id)) != client_map.end() &&
			SDLNet_TCP_Send(it->second->socket, buffer, 19) != 19) 
		{
			remove_client(it->second);
		}
		queue_send_join_area.pop();
	}
	
	buffer[0] = (char)LEAVE_AREA;
	SDLNet_Write16(12,buffer+1);
	while(!queue_send_leave_area.empty()) {
		StructLeaveArea s = queue_send_leave_area.front();
		SDLNet_Write32(s.pos.x, buffer+3);
		SDLNet_Write32(s.pos.y, buffer+7);
		SDLNet_Write32(s.pos.z, buffer+11);
		if((it = client_map.find(s.client_id)) != client_map.end() &&
			SDLNet_TCP_Send(it->second->socket, buffer, 15) != 15) 
		{
			remove_client(it->second);
		}
		queue_send_leave_area.pop();
	}
	
	buffer[0] = (char)UPDATE_BLOCK;
	SDLNet_Write16(17,buffer+1);
	while(!queue_send_update_block.empty()) {
		StructUpdateBlock s = queue_send_update_block.front();
		SDLNet_Write32(s.pos.x, buffer+3);
		SDLNet_Write32(s.pos.y, buffer+7);
		SDLNet_Write32(s.pos.z, buffer+11);
		buffer[15] = s.m;
		SDLNet_Write32(s.rev, buffer+16);
		if((it = client_map.find(s.client_id)) != client_map.end() &&
			SDLNet_TCP_Send(it->second->socket, buffer, 20) != 20) 
		{
			remove_client(it->second);
		}
		queue_send_update_block.pop();
	}
	
	buffer[0] = (char)PLAYER_POSITION;
	SDLNet_Write16(17,buffer+1);
	while(!queue_send_player_position.empty()) {
		StructPlayerPosition s = queue_send_player_position.front();
		SDLNet_Write32(s.playerid, buffer+3);
		((double*)(buffer+7))[0] = s.pos.x;
		((double*)(buffer+7))[1] = s.pos.y;
		((double*)(buffer+7))[2] = s.pos.z;
		((double*)(buffer+7))[3] = s.pos.orientationHorizontal;
		((double*)(buffer+7))[4] = s.pos.orientationVertical;		
		if((it = client_map.find(s.client_id)) != client_map.end() &&
			SDLNet_TCP_Send(it->second->socket, buffer, 20) != 20) 
		{
			remove_client(it->second);
		}
		queue_send_player_position.pop();
	}
		
	SDL_UnlockMutex(mutex);
}

BlockPosition Network::recv_get_area(int* revision, int* connection) {
	SDL_LockMutex(mutex);
	StructGetArea s = queue_recv_get_area.front();
	queue_recv_get_area.pop();
	SDL_UnlockMutex(mutex);
	
	if(revision)   *revision   = s.rev;
	if(connection) *connection = s.client_id;
	
	return s.pos;
}

int Network::recv_push_area(BlockPosition* pos, char* data, int* revision, int length, bool compressed, int* connection) {
	SDL_LockMutex(mutex);
	StructPushArea s = queue_recv_push_area.front();
	queue_recv_push_area.pop();
	SDL_UnlockMutex(mutex);
	
	if(revision)   *revision   = s.rev;
	if(connection) *connection = s.client_id;
	if(pos)        *pos        = s.pos;
	
	uLongf bytes;
	
	if(compressed) {
		memcpy(data, s.data, std::min(s.length, length));
		bytes = std::min(s.length, length);
	} else {
		bytes = length;
		uncompress((Bytef*)data, &bytes, (Bytef*)s.data, s.length);
	}
	
	delete [] s.data;
	
	return bytes;
}

BlockPosition Network::recv_join_area(int* revision, int* connection) {
	SDL_LockMutex(mutex);
	StructJoinArea s = queue_recv_join_area.front();
	queue_recv_join_area.pop();
	SDL_UnlockMutex(mutex);
	
	if(revision)   *revision   = s.rev;
	if(connection) *connection = s.client_id;
	
	return s.pos;
}

BlockPosition Network::recv_leave_area(int* connection) {
	SDL_LockMutex(mutex);
	StructLeaveArea s = queue_recv_leave_area.front();
	queue_recv_leave_area.pop();
	SDL_UnlockMutex(mutex);
	
	if(connection) *connection = s.client_id;
	
	return s.pos;
}

Material Network::recv_update_block(BlockPosition* pos, int* revision, int* connection) {
	SDL_LockMutex(mutex);
	StructUpdateBlock s = queue_recv_update_block.front();
	queue_recv_update_block.pop();
	SDL_UnlockMutex(mutex);
	
	if(pos)        *pos        = s.pos;
	if(revision)   *revision   = s.rev;
	if(connection) *connection = s.client_id;
	
	return s.m;
}


PlayerPosition Network::recv_player_position(int* playerid, int* connection) {
	SDL_LockMutex(mutex);
	StructPlayerPosition s = queue_recv_player_position.front();
	queue_recv_player_position.pop();
	SDL_UnlockMutex(mutex);
	
	if(playerid)   *playerid   = s.playerid;
	if(connection) *connection = s.client_id;
	
	return s.pos;
}


void Network::send_get_area(BlockPosition pos, int revision, int connection) {
	SDL_LockMutex(mutex);
	queue_send_get_area.push(StructGetArea(pos, revision, connection));
	SDL_UnlockMutex(mutex);
}

void Network::send_push_area(BlockPosition pos, int revision, char* data, int length, bool compressed, int connection) {
	char intbuffer[64*1024+3];
	if(!compressed && length) {
		uLongf size = 64*1024+3;
		compress((Bytef*)intbuffer, &size, (Bytef*)data, length);
		data = intbuffer;
		length = size;
	}
	
	SDL_LockMutex(mutex);
	queue_send_push_area.push(StructPushArea(pos, revision, data, length, connection));
	SDL_UnlockMutex(mutex);
}

void Network::send_join_area(BlockPosition pos, int revision, int connection) {
	SDL_LockMutex(mutex);
	queue_send_join_area.push(StructJoinArea(pos, revision, connection));
	SDL_UnlockMutex(mutex);
}

void Network::send_leave_area(BlockPosition pos, int connection) {
	SDL_LockMutex(mutex);
	queue_send_leave_area.push(StructLeaveArea(pos, connection));
	SDL_UnlockMutex(mutex);
}

void Network::send_update_block(BlockPosition pos, Material m, int revision, int connection) {
	SDL_LockMutex(mutex);
	queue_send_update_block.push(StructUpdateBlock(pos, m, revision, connection));
	SDL_UnlockMutex(mutex);
}

void Network::send_player_position(PlayerPosition pos, int playerid, int connection) {
	SDL_LockMutex(mutex);
	queue_send_player_position.push(StructPlayerPosition(pos, playerid, connection));
	SDL_UnlockMutex(mutex);
}
