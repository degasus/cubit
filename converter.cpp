#include <fstream>
#include <string.h>
#include <iostream>
#include <vector>
#include <assert.h>

#include <sqlite3.h>
#include <SDL_thread.h>

#include "zlib.h"
#include "harddisk.h"
#include "utils.h"
#include <queue>

Harddisk* disk;

void parse_file(const char* f) {
	std::ifstream file(f);
	
	file.seekg (0, std::ios::end);
	int length = file.tellg();
	file.seekg (0, std::ios::beg);
	
	unsigned char* buffer = new unsigned char[length];
	file.read((char*) buffer, length);
	file.close();
	
	int state = 0;
	int wert = 0;
	bool neg = 0;
	int string_pos = 0;
	int glob_x = 0;
	int glob_y = 0;
	int glob_z = 0;
	while(f[string_pos]) {
		if(f[string_pos] == '.') { 
			if(state == 1)
				glob_y = 512*wert * (neg?-1:1);
			else if(state == 2)
				glob_x = -512*wert * (neg?-1:1);
			state++; 
			neg = 0; 
			wert = 0;
		} else if (f[string_pos] == '-') neg = !neg;
		else if ('0' <= f[string_pos] && f[string_pos] <= '9') {
			wert = wert*10 + int(f[string_pos] - '0');
		}
		string_pos++;
	}
	
	std::cout << f << " " << glob_x << " " << glob_y << " " << glob_z << std::endl;
	
	int locations[1024];
	int counts[1024];
	int timestamps[1024];
	unsigned char* chunks[1024];
	int chunk_length[1024];
	unsigned char* raw_data[1024];
	
	for(int i=0; i<1024; i++) {
		locations[i]    = (buffer[4*i]<<16) | (buffer[4*i+1]<<8) | (buffer[4*i+2]);
		counts[i]       =  buffer[4*i+3];
		timestamps[i]   = (buffer[4*i+4096]<<24) | (buffer[4*i+1+4096]<<16) | (buffer[4*i+2+4096]<<8) | (buffer[4*i+3+4096]);
		chunks[i]       = buffer + locations[i]*4096 + 5;
		
		if(!counts[i]) {
			raw_data[i] = 0;
			chunk_length[i] = 0;
			continue;
		}
		chunk_length[i] = (chunks[i][-5]<<24) | (chunks[i][-4]<<16) | (chunks[i][-3]<<8) | (chunks[i][-2]);
		
		uLongf size = 16*16*128*16;
		unsigned char out[16*16*128*16];
		uncompress(out, &size, chunks[i], chunk_length[i]);
		
		raw_data[i] = new unsigned char[16*16*128];

		int have = 16*16*128*16-size;
		
		bool found = false;
		for(int k=6; k<have-16*16*128; k++) {
			if(out[k-6] == 'B' && out[k-5] == 'l' && out[k-4] == 'o' && out[k-3] == 'c' && out[k-2] == 'k' && out[k-1] == 's') {
				memcpy(raw_data[i], out+k, 16*16*128);
				found = true;
				break;
			} 
		}
		if(!found) std::cout << "Blocks not found" << std::endl;
		
		//std::cout << locations[i] << " x " << counts[i] << " x " << timestamps[i] << " x " << chunk_length[i] << (void*)raw_data[i] << std::endl;
		
	}
	
	for(int x=0; x<16; x++) for(int y=0; y<16; y++) for(int z=0; z<4; z++) {
		unsigned char area[AREASIZE];
		bool save = 1;
		for(int pos=0; pos < AREASIZE; pos++) {
			int xb = pos/1024;
			int yb = (pos/32)%32;
			int zb = pos%32;
			
			if(zb < 10 && z == 0)
				area[pos] = 7;
			else {
				int xm = y*32+yb;
				int ym = z*32+zb;
				int zm = 16*32-x*32-xb-1;
				
				assert(xm>=0 && ym>=0 && zm>=0 && xm<16*32 && ym<128 && zm<16*32);
				int k = (xm/16) + (zm/16)*32;
				if(raw_data[k]) {
					area[pos] = raw_data[k][((xm%16)<<11) + ((zm%16)<<7) + ym];
				} else {
					save = 0;
				}
			}
		}
		if(save) {
			bool empty = 1;
			
			
			for(int p=0; p<AREASIZE; p++) {
				if(area[p]) {
					empty = 0;
				} else {
				}
			}
			
			BlockPosition pos = BlockPosition::create(glob_x + x*32, glob_y + y*32, glob_z + z*32 - 64);
			if(empty) 
				disk->writeArea(pos, 0, 1);
			else
				disk->writeArea(pos, area, 1);
		}
	}
		
	delete [] buffer;
	for(int i=0; i<1024; i++) {
		if(raw_data[i]) delete [] raw_data[i];
	}
}

std::queue<std::string> files;
const int threads_count = 8;
SDL_Thread* threads[threads_count];
SDL_mutex* lock;

int threaded_convert(void* param) {
	while(true) {
		std::string file;
		bool empty;
		
		SDL_LockMutex(lock);
		empty = files.empty();
		if(!empty){
			file = files.front();
			files.pop();
			}
		SDL_UnlockMutex(lock);
		
		if(empty) break;
		else parse_file(file.c_str());
	}
	return 0;
}

int main(int argc, const char* argv[]) {
	
	disk = new Harddisk();
	
	for(int i=1; i<argc; i++)
		//parse_file(argv[i]);
		files.push(argv[i]);
	
	lock = SDL_CreateMutex();
	
	for(int i=0; i<threads_count; i++)
		threads[i] = SDL_CreateThread(threaded_convert, 0);
	
	int ret;
	for(int i=0; i<threads_count; i++)
		SDL_WaitThread(threads[i], &ret);
	
	delete disk;
	
	return 0;
}