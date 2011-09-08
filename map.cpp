#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <fstream>
#include <string>

#include <stdio.h>
#include <cstdio>


#include "config.h"
#include "controller.h"

#include "map.h"

#include "zlib.h"
#include <SDL_net.h>
#include "harddisk.h"


Map::Map(Controller *controller) {
	c = controller;
	
	harddisk = 0;
        network = 0;
	mapGenerator = 0;
	queue_mutex = 0;
	inital_loaded = 0;
	
	dijsktra_wert = 1;
	
	IPaddress ip;
	if(SDLNet_ResolveHost(&ip,"10.43.2.148",PORT)==-1) {
		printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
		exit(1);
	}

	tcpsock=SDLNet_TCP_Open(&ip);
	if(!tcpsock) {
		printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
		exit(2);
	}
	
	set=SDLNet_AllocSocketSet(16);
	if(!set) {
		printf("SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
		exit(1); //most of the time this is a major error, but do what you want.
	}
	
	// add two sockets to a socket set
	int numused;

	numused=SDLNet_TCP_AddSocket(set,tcpsock);
	if(numused==-1) {
		printf("SDLNet_AddSocket: %s\n", SDLNet_GetError());
		// perhaps you need to restart the set and make it bigger...
	}
	
	
}

Map::~Map()
{
	thread_stop = 1;
	int thread_return = 0;

	if(harddisk)
		SDL_WaitThread (harddisk, &thread_return);
        
        if(network)
          SDL_WaitThread (network, &thread_return);
	
	if(queue_mutex)
		SDL_DestroyMutex(queue_mutex);
	
	iterator it;
	
	for(it = areas.begin(); it != areas.end(); it++) {
		if(storeMaps && it->second->needstore)
			store(it->second);
		
		it->second->delete_collision(c->movement->dynamicsWorld);
		delete it->second;
	}
	
	if(disk) delete disk;
}


void Map::config(const boost::program_options::variables_map& c)
{
	deleteRange = c["visualRange"].as<int>()*c["destroyAreaFaktor"].as<double>()*2;
	storeMaps = c["storeMaps"].as<bool>();
	areasPerFrameLoading = c["areasPerFrameLoading"].as<int>();
	loadRange = c["visualRange"].as<int>()*2+2;
	generate_random = c["generateRandom"].as<bool>();
	
	disk = new Harddisk((c["workingDirectory"].as<boost::filesystem::path>() / "cubit.db").string());	
}

int threaded_read_from_harddisk(void* param) {
	Map* map = (Map*)param;
	
	map->read_from_harddisk();
	
	return 0;
}

int threaded_read_from_network(void* param) {
  Map* map = (Map*)param;
  
  map->read_from_network();
  
  return 0;
}

void Map::randomArea(Area* a) {
	if(a->pos.z > 92) return;
	
	a->allocm();
	a->empty = 0;
	for(int x = a->pos.x; x < AREASIZE_X+a->pos.x; x++)
	for(int y = a->pos.y; y < AREASIZE_Y+a->pos.y; y++)
	for(int z = a->pos.z; z < AREASIZE_Z+a->pos.z; z++){
		int height = cos( ((2*M_PI)/180) * ((int)((x)/0.7) % 180 )) * 8;
		height += sin( ((2*M_PI)/180) * ((int)((y)) % 180 )) * 8;
		height += -sin( ((2*M_PI)/180) * ((int)((x)/2.5) % 180 )) * 25;
		height += -cos( ((2*M_PI)/180) * ((int)((y)/5) % 180 )) * 50;
		Material m;
		
		if(z <  height - 10){
			m = 1 + ((z) % (NUMBER_OF_MATERIALS-1) + (NUMBER_OF_MATERIALS-1)) % (NUMBER_OF_MATERIALS-1);
			if(m==9) m++; // no water
		}
		else if(z <  height - 4){
			m = 1; //stone
		}
		else if(z <  height - 1 - std::rand() % 1){
			if (z >= -65)
				m = 3; //mud
			else
				m = 12; //sand
		}
		else if(z <  height){
			if(z >= 60 - std::rand() % 2)
				m = 82; //snow
			else if (z >= -65 - std::rand() % 2)
				m = 2; //grass
			else
				m = 12; //sand
		} else if(z < -70) {
			m = 9; // water
		} else {
			m = 0; //air
		}
		
		
		a->m[a->getPos(BlockPosition::create(x,y,z))] = m;
		
		assert(a->m[a->getPos(BlockPosition::create(x,y,z))] >= 0);
		assert(a->m[a->getPos(BlockPosition::create(x,y,z))] < NUMBER_OF_MATERIALS);
		
	}
}

void Map::read_from_harddisk() {
	while(!thread_stop) {
		Area* toload = 0;
		bool empty = 1;
		do{
			SDL_LockMutex(queue_mutex);
			if(!empty) {
				loaded_hdd.push(toload);
			}
			empty = to_load_hdd.empty();
			if(!empty) {
				toload = to_load_hdd.front();
				to_load_hdd.pop();
			}
			SDL_UnlockMutex(queue_mutex);
			
			if(!empty)
				load(toload);
		} while(!empty);
		
		empty = 1;
		Area* tosave;
		
		do {
			SDL_LockMutex(queue_mutex);
			empty = to_save_hdd.empty();
			if(!empty) {
				tosave = to_save_hdd.front();
				to_save_hdd.pop();
			}
			SDL_UnlockMutex(queue_mutex);
			
			if(!empty) {
				store(tosave);
				if(tosave->state == Area::STATE_DELETE)
					delete tosave;
			}
			
		} while(!empty);
		
		SDL_Delay (10);
	}
}

void Map::read_from_network() {
  int numready, buffer_usage = 0;
  char buffer[64*1024+3];
  while(!thread_stop) {
    Area* toload = 0;
    bool empty = 1;
    do{
      SDL_LockMutex(queue_mutex);
      empty = to_load_net.empty();
      if(!empty) {
        toload = to_load_net.front();
        to_load_net.pop();
      }
      SDL_UnlockMutex(queue_mutex);
      
      if(!empty)
        request_load_net(toload);
      
    } while(!empty && !SDLNet_SocketReady(tcpsock));
    
    
    do {
      numready=SDLNet_CheckSockets(set, 10);
      if(numready==-1) {
        printf("SDLNet_CheckSockets: %s\n", SDLNet_GetError());
        //most of the time this is a system error, where perror might help you.
        perror("SDLNet_CheckSockets");
      }
      else if(numready>0){
        if(SDLNet_SocketReady(tcpsock)) {
          int result;
          int maxlen = 1;
          if(buffer_usage < 3)
            maxlen = 3-buffer_usage;
          else
            maxlen = 3 + SDLNet_Read16(buffer+1) - buffer_usage;
          
          //printf("want to recv %d bytes\n", maxlen); 
            result=SDLNet_TCP_Recv(tcpsock,buffer+buffer_usage,maxlen);
            if(result<=0) {
              // An error may have occured, but sometimes you can just ignore it
              // It may be good to disconnect socket because it is likely invalid now.
              
              //FIXME: Errorhandling, reconnect, close socket, ...
            }
            else{
              buffer_usage += result;
              //printf("%d recv, %d availible, %d wanted\n", result, client->buffer_usage, 3 + SDLNet_Read16(client->buffer+1));
              if(buffer_usage >= 3 && buffer_usage == 3 + SDLNet_Read16(buffer+1)) {
                //char outputBuffer[64*1024+3];
                int size = buffer_usage-3;
                //int outSize;
                int posx, posy, posz, rev, playerid;
                Material material;
                double pposx, pposy, pposz, pposh, pposv;
                BlockPosition bPos;
                Area *a;
                std::map<BlockPosition, Area*>::iterator it;
                //printf("Data recv: %d bytes, %d type, %s\n", client->buffer_usage ,client->buffer[0],client->buffer+3);
                switch ((Commands)buffer[0]) {
                  case GET_AREA:
                    posx = SDLNet_Read32(buffer+3);
                    posy = SDLNet_Read32(buffer+7);
                    posz = SDLNet_Read32(buffer+11);
                    rev  = SDLNet_Read32(buffer+15);
                    bPos = BlockPosition::create(posx, posy, posz).area();
                    printf("GET_AREA: posx=%d, posy=%d, posz=%d, revision=%d\n", posx, posy, posz, rev);
                    break;
                  case PUSH_AREA:
                    posx = SDLNet_Read32(buffer+3);
                    posy = SDLNet_Read32(buffer+7);
                    posz = SDLNet_Read32(buffer+11);
                    rev  = SDLNet_Read32(buffer+15);
                    bPos = BlockPosition::create(posx, posy, posz).area();
                    //printf("PUSH_AREA: posx=%d, posy=%d, posz=%d, revision=%d\n", posx, posy, posz, rev);
                    it = load_requested_net.find(bPos);
                    if(it!=load_requested_net.end()){
                      a = it->second;
                      a->revision = rev;
                      if(buffer_usage == 19) a->empty = 1;
                      else {
                        a->allocm();
                        a->empty = 0;
                        uLongf dsize = AREASIZE;
                        uncompress(a->m, &dsize, (Bytef*)buffer+19, buffer_usage-19);
                      }
                      a->state = Area::STATE_NET_LOADED;
                      recalc(a);
                      SDL_LockMutex(queue_mutex);
                      loaded_net.push(a);
                      SDL_UnlockMutex(queue_mutex);
                      
                      load_requested_net.erase(it);
                    }
                    break;
                  case JOIN_AREA:
                    posx = SDLNet_Read32(buffer+3);
                    posy = SDLNet_Read32(buffer+7);
                    posz = SDLNet_Read32(buffer+11);
                    rev  = SDLNet_Read32(buffer+15);
                    printf("JOIN_AREA: posx=%d, posy=%d, posz=%d, revision=%d\n", posx, posy, posz, rev);
                    break;
                  case LEAVE_AREA:
                    posx = SDLNet_Read32(buffer+3);
                    posy = SDLNet_Read32(buffer+7);
                    posz = SDLNet_Read32(buffer+11);
                    printf("LEAVE_AREA: posx=%d, posy=%d, posz=%d\n", posx, posy, posz);
                    break;
                  case UPDATE_BLOCK:
                    posx = SDLNet_Read32(buffer+3);
                    posy = SDLNet_Read32(buffer+7);
                    posz = SDLNet_Read32(buffer+11);
                    material = buffer[15];
                    printf("UPDATE_BLOCK: posx=%d, posy=%d, posz=%d, matrial=%d\n", posx, posy, posz, material);
                    break;
                  case PLAYER_POSITION:
                    playerid = SDLNet_Read32(buffer+3);
                    pposx = ((double*)(buffer+7))[0];
                    pposy = ((double*)(buffer+7))[1];
                    pposz = ((double*)(buffer+7))[2];
                    pposh = ((double*)(buffer+7))[3];
                    pposv = ((double*)(buffer+7))[4];
                    printf("PLAYER_POSITION: playerid=%d, posx=%f, posy=%f, posz=%f, posh=%f, posv=%f\n", playerid, pposx, pposy, pposz, pposh, pposv);
                    break;
                  default:
                    printf("UNKNOWN COMMAND\n");
                    break;
                }
                buffer_usage = 0;
              }          
            }
        }
      }
    } while(numready > 0);
  }
}

void Map::init()
{
	thread_stop = 0;
	queue_mutex = SDL_CreateMutex ();
	harddisk = SDL_CreateThread (threaded_read_from_harddisk,this);
        network = SDL_CreateThread (threaded_read_from_network,this);
}

Area* Map::getArea(BlockPosition pos)
{
	iterator it = areas.find(pos.area());
	if(it != areas.end()) {
		if(it->second->state != Area::STATE_READY)
			throw NotLoadedException();
		if(it->second->empty)
			throw AreaEmptyException();
		return it->second;
	} else {
		throw NotLoadedException(); 
	}
}

Area* Map::getOrCreate(BlockPosition pos) {
	Area** a = &areas[pos];
	if(!(*a)) {
		*a = new Area(pos);
		for(int i=0; i<DIRECTION_COUNT; i++) {
			iterator it = areas.find(pos*DIRECTION(i));
			if(it != areas.end()) {
				it->second->next[!DIRECTION(i)] = *a;
				(*a)->next[i] = it->second;
			}
		}
	}
	return *a;
}

void Map::setPosition(PlayerPosition pos)
{	
	SDL_LockMutex(queue_mutex);
        
        //std::cout << "anzahl Areas: " << load_requested_net.size() << std::endl;
	
	while(!loaded_hdd.empty()) {
		
		Area* a = loaded_hdd.front();
		loaded_hdd.pop();
                
                
		
                a->state = Area::STATE_NET_LOAD;
                to_load_net.push(a);
                /*
		if(a->state == Area::STATE_HDD_LOADED){
			areas[a->pos] = a;
			a->state = Area::STATE_READY;
			
			for(int i=0; i<DIRECTION_COUNT; i++)
				if(a->next[i])
					a->next[i]->needupdate = 1;
			
			if(!a->empty)
				areas_with_gllist.insert(a);
		} 
		*/
	}
	
	while(!loaded_net.empty()){
          
          Area* a = loaded_net.front();
          loaded_net.pop();
          
          if(a->state == Area::STATE_NET_LOADED){
            areas[a->pos] = a;
            a->state = Area::STATE_READY;
            
            for(int i=0; i<DIRECTION_COUNT; i++)
              if(a->next[i])
                a->next[i]->needupdate = 1;
              
              if(!a->empty)
                areas_with_gllist.insert(a);
          } 
        }
	
	/*
	while(!saved.empty()) {
		Area* a = saved.front();
		saved.pop();
		
		iterator it = areas.find(a->pos);
		if(it->second->state == Area::STATE_DELETE) {
			delete a;
		}
	}
	*/

	
	BlockPosition p = pos.block().area();
	
	if(lastpos != p) {
		inital_loaded = 0;
		lastpos = p;
	}
	
	if(!inital_loaded) {
		// reset queue
		while(!dijsktra_queue.empty())
			dijsktra_queue.pop();
		
		// load actual position
		Area* a = getOrCreate(p);
		if(a->state == Area::STATE_NEW) {
			a->state = Area::STATE_HDD_LOAD;
			to_load_hdd.push(a);
		}
		// add it to queue		
		dijsktra_queue.push(a);
		a->dijsktra_distance = 0;
		inital_loaded = 1;
		dijsktra_wert++;
		for(int d=0; d<DIRECTION_COUNT; d++)
			a->dijsktra_direction_used[d] = 0;
		//std::cout << "pos: " << pos.to_string() << " " << a->state << std::endl;
	}
	//std::cout << "load: " << to_load.size() << ", store: " << to_save.size() << ", queue: " << dijsktra_queue.size() << std::endl;

	//int maxk = std::min((areasPerFrameLoading - std::max(to_load.size(), to_save.size())), dijsktra_queue.size());
	int maxk = areasPerFrameLoading - std::max(to_load_hdd.size(), to_save_hdd.size()) * 16;
	Area *first = 0;
	for(int k=0; k<maxk && !dijsktra_queue.empty(); k++) {
		Area* a = dijsktra_queue.front();
		
		// do not circle
		if(a == first) break;
		else dijsktra_queue.pop();
		
		switch(a->state) {		
			// found, so go and try any direction
			case Area::STATE_READY:
			if(!a->full) {
				for(int i=0; i<DIRECTION_COUNT; i++) if(!a->dijsktra_direction_used[!((DIRECTION)i)]) {
					Area* b = a->next[i];
					if(!b && a->dijsktra_distance < loadRange) b = getOrCreate(a->pos*DIRECTION(i));
					if(b && (b->dijsktra < dijsktra_wert || b->dijsktra_distance > a->dijsktra_distance+1)) {
						b->dijsktra_distance = a->dijsktra_distance+1;
						b->dijsktra = dijsktra_wert;
						for(int d=0; d<DIRECTION_COUNT; d++)
							b->dijsktra_direction_used[d] = a->dijsktra_direction_used[d] || (i==d);
						dijsktra_queue.push(b);
						if(b->state == Area::STATE_NEW) {
							b->state = Area::STATE_HDD_LOAD;
							to_load_hdd.push(b);
						}
					}
				}
			}
			break;
			
			// wait and try it again later
			case Area::STATE_HDD_LOAD:
                        case Area::STATE_HDD_LOADED:
                        case Area::STATE_HDD_LOADED_BUT_NOT_FOUND:
                        case Area::STATE_NET_LOAD:
                        case Area::STATE_NET_LOADED:
                          
			dijsktra_queue.push(a);
			if(!first) first = a;
			break;
			
                        // not available, so stop here
                        case Area::STATE_NET_LOADED_BUT_NOT_FOUND:
			default: std::cout << "state: " << a->state << std::endl;
		}
		
		if(a->dijsktra_distance > deleteRange && a->state == Area::STATE_READY) {
			a->deconfigure();
			a->delete_collision(c->movement->dynamicsWorld);
			areas_with_gllist.erase(a);
			areas.erase(a->pos);
			a->state = Area::STATE_DELETE;
			to_save_hdd.push(a);
		} else if(a->needstore && a->state == Area::STATE_READY) {
			a->needstore = 0;
			to_save_hdd.push(a);
		}
	} 
	SDL_UnlockMutex(queue_mutex);
}

void Map::areaLoadedIsEmpty(BlockPosition pos)
{

}

void Map::areaLoadedSuccess(Area* area)
{

}

void Map::blockChangedEvent(BlockPosition pos, Material m){

}

void Map::store(Area *a) {
	if(a->empty)
		disk->writeArea(a->pos, 0, a->revision);
	else
		disk->writeArea(a->pos, a->m, a->revision);
}

void Map::load(Area *a) {
	a->allocm();
	int bytes = disk->readArea(a->pos, a->m, &a->revision);
	if(bytes) {
		a->empty = false;
	} else {
		a->empty = true;
		delete [] a->m;
		a->m = 0;
	}
	
	if(a->revision) {
		recalc(a);
		a->state = Area::STATE_HDD_LOADED;
	} else {
		a->state = Area::STATE_HDD_LOADED_BUT_NOT_FOUND;
	}
}

void Map::request_load_net(Area *a) {
  char outbuffer[19];
  outbuffer[0] = 1;
  SDLNet_Write16(16,outbuffer+1);
  SDLNet_Write32(a->pos.x,outbuffer+3);
  SDLNet_Write32(a->pos.y,outbuffer+7);
  SDLNet_Write32(a->pos.z,outbuffer+11);
  SDLNet_Write32(0,outbuffer+15);
  SDLNet_TCP_Send(tcpsock, outbuffer, 19);
  
  load_requested_net[a->pos] = a;
}

void Map::recalc(Area* a) {
	a->blocks = 0;
	
	a->full = 0;
	for(int i=0; i<DIRECTION_COUNT; i++) {
		a->dir_full[i] = 0;
	}
	
	if(!a->empty) {
		for(int i=0; i<DIRECTION_COUNT; i++)
			a->dir_full[i] = 1;
		
		for(int x = 0; x < AREASIZE_X; x++)
		for(int y = 0; y < AREASIZE_Y; y++)
		for(int z = 0; z < AREASIZE_Z; z++) {
			a->blocks += (a->m[a->getPos(BlockPosition::create(x+a->pos.x,y+a->pos.y,z+a->pos.z))] != 0);
			for(int i=0; i<DIRECTION_COUNT; i++)
			if(!a->m[a->getPos(BlockPosition::create(x+a->pos.x,y+a->pos.y,z+a->pos.z))] && (
					(x+DIRECTION_NEXT_BOX[i][0]) & ~(AREASIZE_X-1) ||
					(y+DIRECTION_NEXT_BOX[i][1]) & ~(AREASIZE_Y-1) ||
				(z+DIRECTION_NEXT_BOX[i][2]) & ~(AREASIZE_Z-1)
				)
			) a->dir_full[i] = 0;
		}
		
		a->empty = (a->blocks == 0);
		a->full = (a->blocks == AREASIZE_X*AREASIZE_Y*AREASIZE_Z);
		
		if(a->empty) {
			delete [] a->m;
			a->m = 0;
			a->needstore = 1;
		}
	} 
}
	

Material Map::getBlock(BlockPosition pos){
	iterator it = areas.find(pos.area());
	if(it == areas.end())
		throw NotLoadedException();
	
	if(it->second->state != Area::STATE_READY)
		throw NotLoadedException();
	
	return it->second->get(pos);
}


void Map::setBlock(BlockPosition pos, Material m){
	iterator it = areas.find(pos.area());
	if(it == areas.end())
		throw NotLoadedException();
	
	Area* a = it->second;
	
	if(a->state != Area::STATE_READY)
		throw NotLoadedException();

	if(a->empty && m)
		areas_with_gllist.insert(a);
	a->set(pos,m);
	
}
