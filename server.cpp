#include "SDL_thread.h"
#include <queue>
#include <iostream>
#include <cmath>
#include <zlib.h>
#include <cstdlib>

#include "server.h"
#include "config.h"
#include "utils.h"
#include "harddisk.h"
#include "utils.h"
#include "utils.h"



int Server::randomArea(BlockPosition bPos, char* buffer) {
	if (bPos.z > 92) return 0;
	
	Material internalBuffer[AREASIZE];
	
	bool empty = 1;
	for (int x = bPos.x; x < AREASIZE_X+bPos.x; x++)
		for (int y = bPos.y; y < AREASIZE_Y+bPos.y; y++)
			for (int z = bPos.z; z < AREASIZE_Z+bPos.z; z++) {
				int height = cos( ((2*M_PI)/180) * ((int)((x)/0.7) % 180 )) * 8;
				height += sin( ((2*M_PI)/180) * ((int)((y)) % 180 )) * 8;
				height += -sin( ((2*M_PI)/180) * ((int)((x)/2.5) % 180 )) * 25;
				height += -cos( ((2*M_PI)/180) * ((int)((y)/5) % 180 )) * 50;
				Material m;
				
				if (z <  height - 10) {
					m = 1 + ((z) % (NUMBER_OF_MATERIALS-1) + (NUMBER_OF_MATERIALS-1)) % (NUMBER_OF_MATERIALS-1);
					if (m==9) m++; // no water
				}
				else if (z <  height - 4) {
					m = 1; //stone
				}
				else if (z <  height - 1 - std::rand() % 1) {
					if (z >= -65)
						m = 3; //mud
						else
							m = 12; //sand
				}
				else if (z <  height) {
					if (z >= 60 - std::rand() % 2)
						m = 82; //snow
						else if (z >= -65 - std::rand() % 2)
							m = 2; //grass
							else
								m = 12; //sand
				} else if (z < -70) {
					m = 9; // water
				} else {
					m = 0; //air
				}
				if (m) empty = 0;
				int p = (x-bPos.x)*AREASIZE_Y*AREASIZE_Z + (y-bPos.y)*AREASIZE_Z + (z-bPos.z);
				internalBuffer[p] = m;
			}
			if (empty) return 0;
			
			long unsigned int buffer_size = 64*1024-16;
			compress((unsigned char*)buffer, &buffer_size, internalBuffer, AREASIZE);
			
			return buffer_size;
}

Server::Server(){
	harddisk = new Harddisk();
	network = new Network();
	stop = false;
}

Server::~Server() {
	delete harddisk;
	delete network;
}

int main() {
	SDL_Init(0);
	SDLNet_Init();
	
	Server server;
	server.run();
	return 0;
}

void Server::run() {
	BlockPosition bPos;
	char buffer[64*1024+3];
	char buffer2[AREASIZE];
	int rev, rev2, bytes, connection;
	Material m;
	std::map<BlockPosition, std::set<int> >::iterator it;
	std::set<int>::iterator it2;

	while(!stop) {
		while(!network->recv_get_area_empty()){
			bPos = network->recv_get_area(&rev, &connection);
			bytes = harddisk->readArea(bPos, buffer, &rev2, true, 64*1024+3);
			if(!rev2){
				bytes = randomArea(bPos, buffer);
				rev2 = 1;
				harddisk->writeArea(bPos, buffer, rev2, true, bytes);
			}
			if(rev == rev2)
				network->send_push_area(bPos, 0, 0, 0, true, connection);
			else
				network->send_push_area(bPos, rev2, buffer, bytes, true, connection);
		}
		
		while(!network->recv_push_area_empty())
			network->recv_push_area(&bPos, buffer, &rev, 0, true, &connection);
		
		while(!network->recv_join_area_empty()){
			bPos = network->recv_join_area(&rev, &connection);
			joined_clients[bPos].insert(connection);
			bytes = harddisk->readArea(bPos, buffer, &rev2, true, 64*1024+3);
			if(rev != rev2)
				network->send_push_area(bPos, rev2, buffer, bytes, true, connection);
		}
		
		while(!network->recv_leave_area_empty()){
			bPos = network->recv_leave_area(&connection);
			it = joined_clients.find(bPos);
			if(it != joined_clients.end()){
				it->second.erase(connection);
				if(it->second.empty()){
					joined_clients.erase(it);
				}
			}
		}
		
		while(!network->recv_update_block_empty()){
			m = network->recv_update_block(&bPos, &rev, &connection);
			bytes = harddisk->readArea(bPos.area(),buffer2, &rev);
			if(!bytes) {
				for(int i=0; i<AREASIZE; i++) buffer2[i]=0;
			}
			buffer2[
				(bPos.x-bPos.area().x)*AREASIZE_Y*AREASIZE_Z +
				(bPos.y-bPos.area().y)*AREASIZE_Z +
				(bPos.z-bPos.area().z)] = m;
			harddisk->writeArea(bPos.area(),buffer2, ++rev);
			it = joined_clients.find(bPos.area());
			if(it != joined_clients.end()){
				for(it2 = it->second.begin(); it2 != it->second.end(); it2++) {
					network->send_update_block(bPos, m, rev+1, *it2);
				}
			}
		}
	}
}