#include <string>
#include <SDL_thread.h>
#include <sqlite3.h>

#ifndef _HARDDISK_H_
#define _HARDDISK_H_

#include "config.h"
#include "utils.h"

class Harddisk {
public:	
	Harddisk(std::string filename = "");
	~Harddisk();
	
	void begin();
	void commit();
	
	/** read an area from harddisk
	 * @param pos selects area
	 * @param buffer buffer for writing area
	 * @param revision revision readed, zero if not found
	 * @param compressed should buffer be compressed
	 * @param length size of buffer
	 * @return bytes written in buffer
	 */
	int readArea (BlockPosition pos, char* buffer, int* revision, bool compressed=false, int length=AREASIZE);

	/** write an area to harddisk
	 * @param pos selects area
	 * @param buffer buffer for reading area, zero for empty areas
	 * @param revision revision to write
	 * @param compressed is buffer compressed
	 * @param length size of buffer
	 */
	void writeArea(BlockPosition pos, char* buffer, int  revision, bool compressed=false, int length=AREASIZE);
	
private:
	bool in_transaction;
	
	sqlite3* database;	
	
	// prepared statements
	sqlite3_stmt *saveArea;
	sqlite3_stmt *loadArea;
	
	SDL_mutex* mutex;
};




#endif