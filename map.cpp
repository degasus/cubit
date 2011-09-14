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
}

Map::~Map()
{
	thread_stop = 1;
	int thread_return = 0;

	if(harddisk)
		SDL_WaitThread (harddisk, &thread_return);
	
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
	
	if(network) delete network;
}


void Map::config(const boost::program_options::variables_map& c)
{
	deleteRange = c["visualRange"].as<int>()*c["destroyAreaFaktor"].as<double>()*2;
	storeMaps = c["storeMaps"].as<bool>();
	areasPerFrameLoading = c["areasPerFrameLoading"].as<int>();
	loadRange = c["visualRange"].as<int>()*2+2;

	network = new Network(c["server"].as<std::string>().c_str(),PORT);
	
	disk = new Harddisk((c["workingDirectory"].as<boost::filesystem::path>() / "cubit.db").string());	
}

int threaded_read_from_harddisk(void* param) {
	Map* map = (Map*)param;
	
	map->read_from_harddisk();
	
	return 0;
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

void Map::init()
{
	thread_stop = 0;
	queue_mutex = SDL_CreateMutex ();
	harddisk = SDL_CreateThread (threaded_read_from_harddisk,this);
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
	Area* a;    
        //std::cout << "anzahl Areas: " << load_requested_net.size() << std::endl;
	
	while(!loaded_hdd.empty()) {
		
		a = loaded_hdd.front();
		loaded_hdd.pop();
		a->state = Area::STATE_NET_LOAD;
		network->send_get_area(a->pos, a->revision);
		network->send_join_area(a->pos, a->revision);
	}
	
	BlockPosition bPos;
	char buffer[64*1024+3];
	int rev, rev2, bytes;
	Material m;
	std::map<BlockPosition, Area* >::iterator it;
	std::set<int>::iterator it2;
	
	while(!network->recv_get_area_empty()){
		bPos = network->recv_get_area(&rev);
	}
	
	while(!network->recv_push_area_empty()){
		bytes = network->recv_push_area(&bPos, buffer, &rev);
		a = getOrCreate(bPos);
		a->revision = rev;
		a->state = Area::STATE_READY;
		if(bytes){
			a->allocm();
			memcpy(a->m, buffer, bytes);
			a->empty = 0;
			a->needstore = 1;
			a->needupdate = 1;
			
			areas[a->pos] = a;
		}
		else if(rev>0){
			a->empty = 1;
			a->needstore = 1;
			a->needupdate = 1;
		}
		a->recalc();
		for (int i=0; i<DIRECTION_COUNT; i++)
			if (a->next[i])
				a->next[i]->needupdate = 1;
			
		if (!a->empty)
			areas_with_gllist.insert(a);
		
		dijsktra_queue.push(a);
	}
	
	while(!network->recv_join_area_empty()){
		bPos = network->recv_join_area(&rev);
	}
	
	while(!network->recv_leave_area_empty()){
		bPos = network->recv_leave_area();
	}
	
	while(!network->recv_update_block_empty()){
		m = network->recv_update_block(&bPos, &rev);
		it = areas.find(bPos.area());
		if(it != areas.end()) {
			a = it->second;
			if(a->empty && m)
				areas_with_gllist.insert(a);
			a->set(bPos, m);
			for(int d=0; d<DIRECTION_COUNT && !m; d++) {
				if(a->next[d] && a->next[d]->full && *(a->next[d]) << bPos+DIRECTION(d)) {
					areas_with_gllist.insert(a->next[d]);
				}
			}
			dijsktra_queue.push(a);
		} else {
			std::cout << "NotLoadedException recv_update_block_empty" << std::endl;
		}
	}
	
	/*while (!loaded_net.empty()) {
		Area* a = loaded_net.front();
		loaded_net.pop();
		
		if (a->state == Area::STATE_NET_LOADED) {
			areas[a->pos] = a;
			a->state = Area::STATE_READY;
			
			for (int i=0; i<DIRECTION_COUNT; i++)
				if (a->next[i])
					a->next[i]->needupdate = 1;
				
				if (!a->empty)
					areas_with_gllist.insert(a);
		}
	}*/
	
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
	int maxk = areasPerFrameLoading - std::max(to_load_hdd.size(), to_save_hdd.size());
//	Area *first = 0;
	for(int k=0; k<maxk && !dijsktra_queue.empty(); k++) {
		Area* a = dijsktra_queue.front();
		
		// do not circle
		//if(a == first) 
		//	break;
		//else 
			dijsktra_queue.pop();
		//if(!first) first = a;
		
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
			//dijsktra_queue.push(a);
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
		disk->writeArea(a->pos, (char*)a->m, a->revision);
}

void Map::load(Area *a) {
	a->allocm();
	int bytes = disk->readArea(a->pos, (char*)a->m, &a->revision);
	if(bytes) {
		a->empty = false;
	} else {
		a->empty = true;
		delete [] a->m;
		a->m = 0;
	}
	
	if(a->revision) {
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

	/*if(a->empty && m)
		areas_with_gllist.insert(a);
	a->set(pos,m);*/
	
	network->send_update_block(pos, m);
}
