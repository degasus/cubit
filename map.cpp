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
	for(int i=0; i<mapGenerator_counts; i++)
		if(mapGenerator[i])
			SDL_WaitThread (mapGenerator[i], &thread_return);
	delete [] mapGenerator;
		
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

	mapGenerator_counts = c["generatorThreads"].as<int>();
	
	network = new Network(c["server"].as<std::string>().c_str(),c["nick"].as<std::string>().c_str(),PORT);
	disk = new Harddisk((c["workingDirectory"].as<boost::filesystem::path>() / (c["server"].as<std::string>() + ".db").c_str()).string());	
}

int threaded_read_from_harddisk(void* param) {
	Map* map = (Map*)param;
	
	map->read_from_harddisk();
	
	return 0;
}

int threaded_generator(void *param) {
	Map* map = (Map*)param;
	map->generator();
	return 0;
}

void Map::generator() {
	while(!thread_stop) {
		Area *a;
		SDL_LockMutex(queue_mutex);
		if(to_generate.empty()) {
			a=0;
		} else {
			a=to_generate.front();
			to_generate.pop_front();
		}
		SDL_UnlockMutex(queue_mutex);
		
		if(a) {
			a->recalc_polys();
			a->state = Area::STATE_GENERATED;
		} else {
			SDL_Delay(10);
		}
		
		SDL_LockMutex(queue_mutex);
		if(a) {
			generated.push(a);
		} 
		SDL_UnlockMutex(queue_mutex);
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

void Map::init()
{
	thread_stop = 0;
	queue_mutex = SDL_CreateMutex ();
	harddisk = SDL_CreateThread (threaded_read_from_harddisk,this);
	
	mapGenerator = new SDL_Thread*[mapGenerator_counts];
	for(int i=0; i<mapGenerator_counts; i++)
		mapGenerator[i] = SDL_CreateThread(threaded_generator, this);
}

Area* Map::getArea(BlockPosition pos)
{
	iterator it = areas.find(pos.area());
	if(it != areas.end()) {
		if(it->second->state < Area::STATE_WAITING_FOR_BORDERS || it->second->state > Area::STATE_READY) {
			throw NotLoadedException();
		}
		if(it->second->empty) {
			throw AreaEmptyException();
		}
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

bool distance(Area *a, Area *b) {
	return a->dijsktra_distance < b->dijsktra_distance;
}

void Map::setPosition(PlayerPosition pos)
{	
	SDL_LockMutex(queue_mutex);
	Area* a;
		
	while(!loaded_hdd.empty()) {
		
		a = loaded_hdd.front();
		loaded_hdd.pop();
		a->state = Area::STATE_NET_LOAD;
		network->send_join_area(a->pos, a->revision);
	}
	
	BlockPosition bPos;
	PlayerPosition pPos;
	char buffer[64*1024+3];
	int rev, rev2, bytes, id;
	Material m;
	std::map<BlockPosition, Area* >::iterator it;
	std::set<int>::iterator it2;
	std::map<int, OtherPlayer>::iterator itOtherPlayers;
	std::string str;
	
	while(!network->recv_get_area_empty()){
		bPos = network->recv_get_area(&rev);
	}
	
	while(!network->recv_push_area_empty()){
		bytes = network->recv_push_area(&bPos, buffer, &rev);
		it = areas.find(bPos);
		if(it != areas.end() && it->second->state == Area::STATE_NET_LOAD) {
			a = it->second;
			if(bytes){
				a->revision = rev;
				a->allocm();
				memcpy(a->m, buffer, bytes);
				a->empty = 0;
				
				areas[a->pos] = a;
				
				if(storeMaps) {
					to_save_hdd.push(a);
				}
			}
			else if(rev>0){
				a->empty = 1;
				a->revision = rev;
				
				if(storeMaps) {
					to_save_hdd.push(a);
				}
			} else {
			}
			for (int i=0; i<DIRECTION_COUNT; i++)
				if (a->next[i])
					a->next[i]->needupdate_poly = 1;
				
			a->state = Area::STATE_READY;
			if (!a->empty) {
				if(a->hasallneighbor()) {
					a->state = Area::STATE_GENERATE;
					to_generate.push_back(a);
				} else {
					a->state = Area::STATE_WAITING_FOR_BORDERS;
				}
			}
			
			for (int i=0; i<DIRECTION_COUNT; i++) if(a->next[i]) {
				if( a->next[i]->state == Area::STATE_WAITING_FOR_BORDERS &&
					a->next[i]->hasallneighbor()) {
					to_generate.push_back(a->next[i]);
					a->next[i]->state = Area::STATE_GENERATE;
				}
			}
			
			a->recalc();
			if(a->state >= Area::STATE_WAITING_FOR_BORDERS) {
				dijsktra_queue.push(a);
			} else
				std::cout << "unknown state in recv_push: " << a->state << std::endl;
		} else {
			std::cout << "push area recv but no get area send, state: " << a->state << std::endl;
		}
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
			if(a->empty && m) {
				areas_with_gllist.remove(a);
				areas_with_gllist.push_back(a);
			}
			a->set(bPos, m);
			for(int d=0; d<DIRECTION_COUNT && !m; d++) {
				if(a->next[d] && a->next[d]->full && *(a->next[d]) << bPos+DIRECTION(d)) {
					areas_with_gllist.remove(a->next[d]);
					areas_with_gllist.push_back(a->next[d]);
				}
			}
			if(a->state >= Area::STATE_WAITING_FOR_BORDERS)
				dijsktra_queue.push(a);
			else
				std::cout << "unknown state in update_block: " << a->state << std::endl;
		} else {
			std::cout << "NotLoadedException recv_update_block_empty" << std::endl;
		}
	}
	
	while(!network->recv_player_position_empty()){
		pPos = network->recv_player_position(&id);
		itOtherPlayers = otherPlayers.find(id);
		if(itOtherPlayers != otherPlayers.end()) {
			itOtherPlayers->second.pos = pPos;
			itOtherPlayers->second.visible = 1;
		}
	}
	
	while(!network->recv_hello_empty()){
		str = network->recv_hello(&id);
		otherPlayers[id] = OtherPlayer(PlayerPosition(), str, 0);
	}
	
	while(!generated.empty()) {
		Area *a = generated.front();
		generated.pop();
		a->state = Area::STATE_READY;
		areas_with_gllist.remove(a);
		areas_with_gllist.push_back(a);
	}
	
	network->send_player_position(pos);;
	
	BlockPosition p = pos.block().area();
	
	if(lastpos != p) {
		inital_loaded = 0;
		lastpos = p;
	}
	
	if(!inital_loaded) {
		//std::cout << "start dijstra at " << p.to_string() << std::endl;
		// load actual position
		Area* a = getOrCreate(p);
		if(a->state == Area::STATE_NEW) {
			if(storeMaps) {
				a->state = Area::STATE_HDD_LOAD;
				to_load_hdd.push(a);
			} else {
				a->state = Area::STATE_NET_LOAD;
				network->send_join_area(a->pos, a->revision);
			}
		} else if(a->state >= Area::STATE_WAITING_FOR_BORDERS){
			// add it to queue		
			dijsktra_queue.push(a);
		} else {
			std::cout << "FIXME: unknown statein initial found. state " << a->state << std::endl;
		}
		a->dijsktra_distance = 0;
		inital_loaded = 1;
		dijsktra_wert++;
		for(int d=0; d<DIRECTION_COUNT; d++)
			a->dijsktra_direction_used[d] = 0;
		//std::cout << "pos: " << pos.to_string() << " " << a->state << std::endl;
	}
	//std::cout << "load: " << to_load.size() << ", store: " << to_save.size() << ", queue: " << dijsktra_queue.size() << std::endl;

	int maxk = (areasPerFrameLoading - std::max(to_load_hdd.size(), to_save_hdd.size()))*16;
	int k;
	for(k=0; k<maxk && !dijsktra_queue.empty(); k++) {
		Area* a = dijsktra_queue.front();
		dijsktra_queue.pop();
		
		switch(a->state) {		
			// found, so go and try any direction
			case Area::STATE_READY:
			case Area::STATE_GENERATED:
			case Area::STATE_GENERATE:
			case Area::STATE_WAITING_FOR_BORDERS:
			if(!a->full) {
				for(int i=0; i<DIRECTION_COUNT; i++) if(!a->dijsktra_direction_used[!((DIRECTION)i)]) {
					Area* b = a->next[i];
					if(!b && a->dijsktra_distance < loadRange) b = getOrCreate(a->pos*DIRECTION(i));
					if(b && (b->dijsktra != dijsktra_wert || b->dijsktra_distance > a->dijsktra_distance+1)) {
						b->dijsktra_distance = a->dijsktra_distance+1;
						b->dijsktra = dijsktra_wert;
						for(int d=0; d<DIRECTION_COUNT; d++)
							b->dijsktra_direction_used[d] = a->dijsktra_direction_used[d] || (i==d);
						if(b->state == Area::STATE_NEW) {
							if(storeMaps) {
								b->state = Area::STATE_HDD_LOAD;
								to_load_hdd.push(b);
							} else {
								b->state = Area::STATE_NET_LOAD;
								network->send_join_area(b->pos, b->revision);
							}
						} else if(b->state >= Area::STATE_WAITING_FOR_BORDERS) {
							dijsktra_queue.push(b);
						} else {
							// do nothing, will be added automaticly
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
			std::cout << "known state, but should not happen: " << a->state << std::endl;
			break;
			
			// not available, so stop here
			case Area::STATE_NET_LOADED_BUT_NOT_FOUND:
			default: std::cout << "unknown state: " << a->state << std::endl;
		}
		
		if(a->dijsktra_distance > deleteRange && a->state == Area::STATE_READY) {
			network->send_leave_area(a->pos);
			a->deconfigure();
			a->delete_collision(c->movement->dynamicsWorld);
			areas_with_gllist.remove(a);
			areas.erase(a->pos);
			a->state = Area::STATE_DELETE;
			if(a->needstore && storeMaps) {
				to_save_hdd.push(a);
				a->needstore = 0;
			} else {
				delete a;
			}
		} else if(a->needstore && a->state == Area::STATE_READY) {
			a->needstore = 0;
			if(storeMaps)
				to_save_hdd.push(a);
		}
	} 
	//std::cout << "dijsktra usage: " << k << std::endl;
	SDL_UnlockMutex(queue_mutex);
	
	areas_with_gllist.sort(distance);
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
	
	if(it->second->state < Area::STATE_WAITING_FOR_BORDERS || it->second->state > Area::STATE_READY)
		throw NotLoadedException();
	
	return it->second->get(pos);
}


void Map::setBlock(BlockPosition pos, Material m){
	iterator it = areas.find(pos.area());
	if(it == areas.end())
		throw NotLoadedException();
	
	Area* a = it->second;
	
	if(it->second->state < Area::STATE_WAITING_FOR_BORDERS || it->second->state > Area::STATE_READY)
		throw NotLoadedException();

	/*if(a->empty && m)
		areas_with_gllist.insert(a);
	a->set(pos,m);*/
	
	network->send_update_block(pos, m);
}

std::string Map::debug_msg() {
	std::ostringstream o;
	
	size_t areas_self = 0;
	size_t areas_materials = 0;
	size_t areas_polys = 0;
	size_t areas_vbo = 0;
	
	int states[12];
	for(int i=0; i<12; i++) {
		states[i] = 0;
	}
	
	for(iterator it=areas.begin(); it!=areas.end(); it++) {
		areas_self += sizeof(Area);
		if(!it->second->empty)
			areas_materials += AREASIZE;
		for(int i=0; i<NUMBER_OF_LISTS; i++) {
			areas_polys += it->second->polys_list_size[i] * sizeof(polygon);
			areas_vbo += it->second->vbo_length[i];
		}
		
		
		states[it->second->state]++;
	}
	areas_self /= 1024*1024;
	areas_materials /= 1024*1024;
	areas_polys /= 1024*1024;
	areas_vbo /= 1024*1024;
	
	std::string strings[12] = {"new", "hddload", "hddloaded", "hddnotfound", 
								"netload", "netloaded", "netnotfound",
								"waiting", "generate", "generated", "ready", "delete"
	};

	o << "Map: " << areas_self << "+" << areas_materials << "+" << areas_polys << "+" << areas_vbo << " MB";
	o << ", Dijstra: " << dijsktra_queue.size() << ", States: ";	
	for(int i=0; i<12; i++) {
		if(states[i])
			o << strings[i] << " " << states[i] << ", ";
	}
	return o.str();
}
