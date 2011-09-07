#include "SDL_net.h"
#include <queue>
#include <sqlite3.h>
#include <iostream>
#include <cmath>
#include <zlib.h>
#include <cstdlib>
#include <signal.h>

#include "server.h"
#include "config.h"

sqlite3* database;
sqlite3_stmt *loadArea;

void initSQL() {
  // init SQL
  if(sqlite3_open("cubit.db", &database) != SQLITE_OK)
    // Es ist ein Fehler aufgetreten!
  std::cout << "Fehler beim Öffnen: " << sqlite3_errmsg(database) << std::endl;
  
  // create tables
  sqlite3_exec(database,
               "CREATE TABLE IF NOT EXISTS area ( "
               "posx INT NOT NULL, "
               "posy INT NOT NULL, "
               "posz INT NOT NULL, "
               "empty BOOL NOT NULL DEFAULT 0, "
               "revision INT DEFAULT 0, "
               "full INT NOT NULL DEFAULT 0, " 
               "blocks INT NOT NULL DEFAULT -1, "
               "data BLOB(32768), "
               "PRIMARY KEY (posx, posy, posz) "
               ");"
               , 0, 0, 0);
  sqlite3_exec(database, "PRAGMA synchronous = 0;", 0, 0, 0);
  
  if (sqlite3_prepare_v2(
    database,            /* Database handle */
    "SELECT empty, revision, full, blocks, data from area where posx = ? and posy = ? and posz = ?;",       /* SQL statement, UTF-8 encoded */
    -1,              /* Maximum length of zSql in bytes. */
    &loadArea,  /* OUT: Statement handle */
    0     /* OUT: Pointer to unused portion of zSql */
  ) != SQLITE_OK)
    std::cout << "prepare(loadArea) hat nicht geklappt: " << sqlite3_errmsg(database) << std::endl;
    
  
}

int randomArea(BlockPosition bPos, char* buffer) {
  if(bPos.z > 92) return 0;
  
  Material internalBuffer[AREASIZE];
  
  bool empty = 1;
  for(int x = bPos.x; x < AREASIZE_X+bPos.x; x++)
    for(int y = bPos.y; y < AREASIZE_Y+bPos.y; y++)
      for(int z = bPos.z; z < AREASIZE_Z+bPos.z; z++){
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
        if(m) empty = 0;
        int p = (x-bPos.x)*AREASIZE_Y*AREASIZE_Z + (y-bPos.y)*AREASIZE_Z + (z-bPos.z);
        internalBuffer[p] = m;        
      }
      if(empty) return 0;
      
      //ZLIB
      // deflate
      z_stream strm;
      int out_usage;
      
      /* allocate inflate state */
      strm.zalloc = Z_NULL;
      strm.zfree = Z_NULL;
      strm.opaque = Z_NULL;
      strm.avail_in = 0;
      strm.next_in = Z_NULL;
      if (deflateInit(&strm, -1) != Z_OK)
        std::cout << "fehler" << std::endl;
      
      /* decompress until deflate stream ends or end of file */
      strm.avail_in = AREASIZE;
      strm.next_in = internalBuffer;
      strm.avail_out = AREASIZE;
      strm.next_out = (Bytef*)buffer;
      
      if(deflate(&strm, Z_FINISH) != Z_STREAM_END)
        std::cout << "fehlerb" << std::endl;
      out_usage = AREASIZE-strm.avail_out;
      deflateEnd(&strm);
      
      return out_usage;
}

int load(BlockPosition bPos, int* revision, char* buffer) {
  bool empty = true;
  *revision = 0;
  int bytes = 0;
  sqlite3_bind_int(loadArea, 1, bPos.x);
  sqlite3_bind_int(loadArea, 2, bPos.y);
  sqlite3_bind_int(loadArea, 3, bPos.z);
  int step = sqlite3_step(loadArea);
  if (step == SQLITE_ROW) {
    empty = sqlite3_column_int(loadArea, 0);
    *revision = sqlite3_column_int(loadArea, 1);
    if(!empty) {
      bytes = sqlite3_column_bytes16(loadArea, 4);
      memcpy(buffer,sqlite3_column_blob(loadArea, 4),bytes);
    }
    sqlite3_reset(loadArea);
  } 
  else if (step == SQLITE_DONE) {
    sqlite3_reset(loadArea);
    *revision = 1;
    bytes = randomArea(bPos, buffer); 
    empty = !bytes;
  } 
  else  {
    std::cout << "SQLITE ERROR" << std::endl;
    sqlite3_reset(loadArea);
  }
  
  if(empty)
    return 0;
  else
    return bytes;
}

int main() {
  signal(SIGPIPE, SIG_IGN);
  initSQL();
  
  IPaddress *remote_ip;
  IPaddress ip;
  TCPsocket tcpsock;
  Client* client;
  std::queue<Client*> clients;

  if(SDLNet_ResolveHost(&ip,NULL,PORT)==-1) {
      printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
      exit(1);
  }

  tcpsock=SDLNet_TCP_Open(&ip);
  if(!tcpsock) {
      printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
      exit(2);
  }
   	
  // Create a socket set to handle up to 16 sockets
  SDLNet_SocketSet set;

  set=SDLNet_AllocSocketSet(MAXCLIENTS);
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
  
  while(true){
    // Wait for up to 1 second for network activity
    //SDLNet_SocketSet set;
    int numready;

    numready=SDLNet_CheckSockets(set, 1000);
    if(numready==-1) {
	printf("SDLNet_CheckSockets: %s\n", SDLNet_GetError());
	//most of the time this is a system error, where perror might help you.
	perror("SDLNet_CheckSockets");
    }
    //printf("There are %d sockets with activity!\n",numready);

    if(numready>0){
      // check all sockets with SDLNet_SocketReady and handle the active ones.
      if(SDLNet_SocketReady(tcpsock)) {
        client = new Client();
        client->buffer_usage = 0;
	client->socket=SDLNet_TCP_Accept(tcpsock);
	if(client->socket) {
	    clients.push(client);
            
            
            remote_ip=SDLNet_TCP_GetPeerAddress(client->socket);
            if(!remote_ip) {
              printf("SDLNet_TCP_GetPeerAddress: %s\n", SDLNet_GetError());
              printf("This may be a server socket.\n");
            }
            printf("Added client %d.%d.%d.%d:%d to list!\n",remote_ip->host%256,remote_ip->host/(256)%256,remote_ip->host/(256*256)%256,remote_ip->host/(256*256*256),remote_ip->port);
	    numused=SDLNet_TCP_AddSocket(set,client->socket);
	    if(numused==-1) {
		printf("SDLNet_AddSocket: %s\n", SDLNet_GetError());
		// perhaps you need to restart the set and make it bigger...
	    }
	}
	else{
          delete client;
        }
      }
      for(int i = 0; i < clients.size() && !clients.empty(); i++){
        client = clients.front();
        clients.pop();
        bool toRemove = false;
        if(SDLNet_SocketReady(client->socket)) {
          int result;
          int maxlen = 1;
          if(client->buffer_usage < 3)
            maxlen = 3-client->buffer_usage;
          else
            maxlen = 3 + SDLNet_Read16(client->buffer+1) - client->buffer_usage;
          
          //printf("want to recv %d bytes\n", maxlen); 
          result=SDLNet_TCP_Recv(client->socket,client->buffer+client->buffer_usage,maxlen);
          if(result<=0) {
            // An error may have occured, but sometimes you can just ignore it
            // It may be good to disconnect socket because it is likely invalid now.
            toRemove = true;
          }
          else{
            client->buffer_usage += result;
            //printf("%d recv, %d availible, %d wanted\n", result, client->buffer_usage, 3 + SDLNet_Read16(client->buffer+1));
            if(client->buffer_usage >= 3 && client->buffer_usage == 3 + SDLNet_Read16(client->buffer+1)) {
              char* buffer = client->buffer + 3;
              char outputBuffer[64*1024+3];
              int size = client->buffer_usage-3;
              int outSize;
              int posx, posy, posz, rev, playerid;
              Material material;
              double pposx, pposy, pposz, pposh, pposv;
              BlockPosition bPos;
              //printf("Data recv: %d bytes, %d type, %s\n", client->buffer_usage ,client->buffer[0],client->buffer+3);
              switch ((Commands)client->buffer[0]) {
                case GET_AREA:
                  posx = SDLNet_Read32(buffer);
                  posy = SDLNet_Read32(buffer+4);
                  posz = SDLNet_Read32(buffer+8);
                  rev  = SDLNet_Read32(buffer+12);
                  bPos = BlockPosition::create(posx, posy, posz).area();
                  //printf("GET_AREA: posx=%d, posy=%d, posz=%d, revision=%d\n", posx, posy, posz, rev);
                  outputBuffer[0] = (char)PUSH_AREA;
                  SDLNet_Write32(bPos.x, outputBuffer+3);
                  SDLNet_Write32(bPos.y, outputBuffer+7);
                  SDLNet_Write32(bPos.z, outputBuffer+11);                  
                  outSize = load(bPos, &rev, outputBuffer+19);
                  SDLNet_Write32(rev, outputBuffer+15);
                  SDLNet_Write16(outSize+16,outputBuffer+1);
                  if(SDLNet_TCP_Send(client->socket,outputBuffer,outSize+19) != outSize+19) {
                    printf("SDLNet_TCP_Send: %s\n", SDLNet_GetError());
                    toRemove = true;
                    // It may be good to disconnect sock because it is likely invalid now.
                  }
                  //printf("send: PUSH_AREA: posx=%d, posy=%d, posz=%d, revision=%d, len(data)=%d\n", bPos.x, bPos.y, bPos.z, rev, outSize);
                  //std::cout << "data " << outputBuffer+19 << " ENDE" << std::endl;
                  break;
                case PUSH_AREA:
                  printf("PUSH_AREA\n");
                  break;
                case JOIN_AREA:
                  posx = SDLNet_Read32(buffer);
                  posy = SDLNet_Read32(buffer+4);
                  posz = SDLNet_Read32(buffer+8);
                  rev  = SDLNet_Read32(buffer+12);
                  printf("JOIN_AREA: posx=%d, posy=%d, posz=%d, revision=%d\n", posx, posy, posz, rev);
                  break;
                case LEAVE_AREA:
                  posx = SDLNet_Read32(buffer);
                  posy = SDLNet_Read32(buffer+4);
                  posz = SDLNet_Read32(buffer+8);
                  printf("LEAVE_AREA: posx=%d, posy=%d, posz=%d\n", posx, posy, posz);
                  break;
                case UPDATE_BLOCK:
                  posx = SDLNet_Read32(buffer);
                  posy = SDLNet_Read32(buffer+4);
                  posz = SDLNet_Read32(buffer+8);
                  material = buffer[12];
                  printf("UPDATE_BLOCK: posx=%d, posy=%d, posz=%d, matrial=%d\n", posx, posy, posz, material);
                  break;
                case PLAYER_POSITION:
                  playerid = SDLNet_Read32(buffer);
                  pposx = ((double*)(buffer+4))[0];
                  pposy = ((double*)(buffer+4))[1];
                  pposz = ((double*)(buffer+4))[2];
                  pposh = ((double*)(buffer+4))[3];
                  pposv = ((double*)(buffer+4))[4];
                  printf("PLAYER_POSITION: playerid=%d, posx=%f, posy=%f, posz=%f, posh=%f, posv=%f\n", playerid, pposx, pposy, pposz, pposh, pposv);
                  break;
                default:
                  printf("UNKNOWN COMMAND\n");
                  break;
              }
              client->buffer_usage = 0;
            }          
          }
        }
        if(!toRemove)
          clients.push(client);
        else{
          numused=SDLNet_TCP_DelSocket(set,client->socket);
          if(numused==-1) {
            printf("SDLNet_DelSocket: %s\n", SDLNet_GetError());
            // perhaps the socket is not in the set
          }
          printf("Client disconnected\n");    
          SDLNet_TCP_Close(client->socket);
          delete client;
        }
      }
    }
  }
}