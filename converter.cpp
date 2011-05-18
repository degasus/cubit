#include <fstream>
#include <string.h>
#include <iostream>
#include <vector>
#include <assert.h>

#include <boost/filesystem.hpp>
#include <sqlite3.h>

#include "lzo/lzoconf.h"
#include "lzo/lzo1x.h"
#include "zlib.h"

namespace fs = boost::filesystem;


void parse_file(const char* f) {
	std::ifstream file(f);
	
	file.seekg (0, std::ios::end);
	int length = file.tellg();
	file.seekg (0, std::ios::beg);
	
	unsigned char* buffer = new unsigned char[length];
	file.read((char*) buffer, length);
	file.close();
	
	// init SQL
	sqlite3* database;
	sqlite3_stmt *saveArea;
	fs::path home = fs::path(std::getenv("HOME"));
	if(sqlite3_open((home / ".cubit" / "cubit.db").string().c_str(), &database) != SQLITE_OK)
		// Es ist ein Fehler aufgetreten!
		std::cout << "Fehler beim Öffnen: " << sqlite3_errmsg(database) << std::endl;
	
	if (sqlite3_prepare_v2(
		database,            /* Database handle */
		"INSERT OR REPLACE INTO area (posx, posy, posz, empty, revision, full, blocks, data) VALUES (?,?,?,?,?,?,?,?);",       /* SQL statement, UTF-8 encoded */
		-1,              /* Maximum length of zSql in bytes. */
		&saveArea,  /* OUT: Statement handle */
		0     /* OUT: Pointer to unused portion of zSql */
	) != SQLITE_OK)
		std::cout << "prepare(saveArea) hat nicht geklappt: " << sqlite3_errmsg(database) << std::endl;
		
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
		
		// inflate
		z_stream strm;
		unsigned char out[16*16*128*16];
		int have;
		raw_data[i] = new unsigned char[16*16*128];
		
		/* allocate inflate state */
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		if (inflateInit(&strm) != Z_OK)
			std::cout << "fehler" << std::endl;
		
		/* decompress until deflate stream ends or end of file */
		strm.avail_in = chunk_length[i];
		strm.next_in = chunks[i];
		strm.avail_out = 16*16*128*16;
		strm.next_out = out;
		
		if(inflate(&strm, Z_NO_FLUSH) != Z_STREAM_END)
			std::cout << "fehlerb" << std::endl;
		have = 16*16*128*16-strm.avail_out;
		inflateEnd(&strm);
		
		for(int k=6; k<have-16*16*128; k++) {
			if(out[k-6] == 'B' && out[k-5] == 'l' && out[k-4] == 'o' && out[k-3] == 'c' && out[k-2] == 'k' && out[k-1] == 's') {
				memcpy(raw_data[i], out+k, 16*16*128);
			}
		}
		
		//std::cout << locations[i] << " x " << counts[i] << " x " << timestamps[i] << " x " << chunk_length[i] << (void*)raw_data[i] << std::endl;
		
	}
	
	sqlite3_exec(database, "BEGIN",  0,0,0);
	
	for(int x=0; x<16; x++) for(int y=0; y<16; y++) for(int z=0; z<4; z++) {
		unsigned char area[32*32*32];
		bool save = 1;
		for(int pos=0; pos < 32*32*32; pos++) {
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
			int blocks = 0;
			int full = 1;
			bool empty = 1;
			
			
			for(int p=0; p<32*32*32; p++) {
				if(area[p]) {
					empty = 0;
					blocks++;
				} else {
					full = 0;
				}
			}
			
			sqlite3_bind_int(saveArea, 1, glob_x + x*32);
			sqlite3_bind_int(saveArea, 2, glob_y + y*32);
			sqlite3_bind_int(saveArea, 3, glob_z + z*32);
			sqlite3_bind_int(saveArea, 4, empty);
			sqlite3_bind_int(saveArea, 5, 1);
			sqlite3_bind_int(saveArea, 6, full);
			sqlite3_bind_int(saveArea, 7, -1);
			
			if(empty)
				sqlite3_bind_null(saveArea, 8);
			else {
			
			unsigned char wrkmem[LZO1X_1_MEM_COMPRESS];
			unsigned char lzobuffer[32*32*32 + 32*32*32 / 16 + 64 + 3];
			lzo_uint buffer_usage;
			
			int r = lzo1x_1_compress(area,32*32*32,lzobuffer,&buffer_usage,wrkmem);

			if(buffer_usage < 32*32*32)
				sqlite3_bind_blob(saveArea, 8, (const void*) lzobuffer, buffer_usage, SQLITE_STATIC);
			else
				sqlite3_bind_blob(saveArea, 8, (const void*) area, 32*32*32, SQLITE_STATIC);
			}
			sqlite3_step(saveArea);
			sqlite3_reset(saveArea);
		}
	}
		
	delete [] buffer;
	sqlite3_exec(database, "COMMIT",  0,0,0);
	sqlite3_close(database);
	for(int i=0; i<1024; i++) {
		if(raw_data[i]) delete [] raw_data[i];
	}
}


int main(int argc, const char* argv[]) {
	
	for(int i=1; i<argc; i++)
		parse_file(argv[i]);
	
	return 0;
}