#include <sqlite3.h>
#include <SDL_thread.h>
#include <iostream>
#include <zlib.h>
#include <boost/filesystem.hpp>

#include "harddisk.h"

#include "config.h"
#include "utils.h"

namespace fs = boost::filesystem;

Harddisk::Harddisk(std::string filename)
{
	if(filename == "") {
		fs::path home = fs::path(std::getenv("HOME"));
		filename = (home / ".cubit" / "cubit.db").string();
	}
	
	std::cout << "open sqlite file in " << filename << std::endl;
	
	in_transaction = false;
	
	// init SQL
	if(sqlite3_open(filename.c_str(), &database) != SQLITE_OK)
	// Es ist ein Fehler aufgetreten!
	std::cout << "Fehler beim Öffnen: " << sqlite3_errmsg(database) << std::endl;
	
	// create tables
	sqlite3_exec(database,
		"CREATE TABLE IF NOT EXISTS area ( "
			"posx INT NOT NULL, "
			"posy INT NOT NULL, "
			"posz INT NOT NULL, "
			"revision INT DEFAULT 0, "
			"data BLOB(32768), "
			"PRIMARY KEY (posx, posy, posz) "
		");"
		, 0, 0, 0);
	sqlite3_exec(database, "PRAGMA synchronous = 0;", 0, 0, 0);
	
	if (sqlite3_prepare_v2(
		database,            /* Database handle */
		"INSERT OR REPLACE INTO area (posx, posy, posz, revision, data) VALUES (?,?,?,?,?);",       /* SQL statement, UTF-8 encoded */
		-1,              /* Maximum length of zSql in bytes. */
		&saveArea,  /* OUT: Statement handle */
		0     /* OUT: Pointer to unused portion of zSql */
	) != SQLITE_OK)
		std::cout << "prepare(saveArea) hat nicht geklappt: " << sqlite3_errmsg(database) << std::endl;
		
	if (sqlite3_prepare_v2(
		database,            /* Database handle */
		"SELECT revision, data from area where posx = ? and posy = ? and posz = ?;",       /* SQL statement, UTF-8 encoded */
		-1,              /* Maximum length of zSql in bytes. */
		&loadArea,  /* OUT: Statement handle */
		0     /* OUT: Pointer to unused portion of zSql */
	) != SQLITE_OK)
		std::cout << "prepare(loadArea) hat nicht geklappt: " << sqlite3_errmsg(database) << std::endl;

	mutex = SDL_CreateMutex();
}

Harddisk::~Harddisk()
{
	//std::cout << "closing database" << std::endl;
	commit();
	
	if(database) sqlite3_close(database);	
	
	if(saveArea) sqlite3_finalize(saveArea);
	if(loadArea) sqlite3_finalize(loadArea);
	
	if(mutex) SDL_DestroyMutex(mutex);
}

int Harddisk::readArea(BlockPosition pos, unsigned char* buffer, int* revision, bool compressed, int length)
{
	//std::cout << "read Area: " << pos.to_string() << std::endl;
	int bytes = 0;
	if(revision) *revision=0;
	SDL_LockMutex(mutex);
	sqlite3_bind_int(loadArea, 1, pos.x);
	sqlite3_bind_int(loadArea, 2, pos.y);
	sqlite3_bind_int(loadArea, 3, pos.z);
	int step = sqlite3_step(loadArea);
	
	// found
	if (step == SQLITE_ROW) {
		if(revision)
			*revision = sqlite3_column_int(loadArea, 0);
		bytes = sqlite3_column_bytes16(loadArea, 1);
		
		if(bytes) {
			if(compressed) {
				memcpy(buffer,sqlite3_column_blob(loadArea, 1),std::min(bytes,length));
			} else {
				uLongf dsize = length;
				uncompress(buffer, &dsize, (unsigned char*)sqlite3_column_blob(loadArea, 1), bytes);
				bytes = dsize;
			}
		}
		
	// not found, do nothing
	} else if (step == SQLITE_DONE) {
	} else  {
		std::cout << "SQLITE ERROR: " << step << std::endl;
	}
	
	sqlite3_reset(loadArea);
	SDL_UnlockMutex(mutex);

	return bytes;
}

void Harddisk::writeArea(BlockPosition pos, unsigned char* buffer, int revision, bool compressed, int length)
{
	//std::cout << "write Area: " << pos.to_string() << std::endl;
	unsigned char intbuffer[AREASIZE];
	
	if(buffer && !compressed) {
		uLongf buffersize = AREASIZE;
		compress(intbuffer, &buffersize, buffer, length);
		buffer = intbuffer;
		length = buffersize;
	}
	
	SDL_LockMutex(mutex);
	sqlite3_bind_int(saveArea, 1, pos.x);
	sqlite3_bind_int(saveArea, 2, pos.y);
	sqlite3_bind_int(saveArea, 3, pos.z);
	sqlite3_bind_int(saveArea, 4, revision);
	
	if(buffer) {
		sqlite3_bind_blob(saveArea, 5, (const void*) buffer, length, SQLITE_STATIC);
	} else {
		sqlite3_bind_null(saveArea, 5);
	}
	
	sqlite3_step(saveArea);
	sqlite3_reset(saveArea);
	
	SDL_UnlockMutex(mutex);
}

void Harddisk::begin()
{
	//std::cout << "begin transaction" << std::endl;
	SDL_LockMutex(mutex);
	if(!in_transaction)
		sqlite3_exec(database, "BEGIN",  0,0,0);
	in_transaction = true;
	SDL_UnlockMutex(mutex);
}

void Harddisk::commit()
{
	//std::cout << "commit transaction" << std::endl;
	SDL_LockMutex(mutex);
	if(in_transaction)
		sqlite3_exec(database, "COMMIT",  0,0,0);
	in_transaction = false;
	SDL_UnlockMutex(mutex);

}
