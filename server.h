#include "SDL_net.h"
#include "config.h"

const int PORT = 1337;
const int MAXCLIENTS = 256;

enum Commands {
  GET_AREA = 1,
  PUSH_AREA = 2,
  JOIN_AREA = 3,
  LEAVE_AREA = 4,
  UPDATE_BLOCK = 5,
  PLAYER_POSITION = 6,
};

struct Client{
  TCPsocket socket;
  char buffer[64*1024+3];
  int buffer_usage;
  
  
};

/**
 * Definiert die Position eines Blocks
 * @param x x Position (West -> Ost)
 * @param y y Position (Süd -> Nord)
 * @param z x Position (Unten -> Oben)
 */ 
struct BlockPosition {
  
  /**
   * Will create the position at the Point (x,y,z)
   */
  static inline BlockPosition create(int x, int y, int z) { 
    BlockPosition b; 
    b.x=x; 
    b.y=y; 
    b.z=z; 
    return b; 
  }
  
  inline bool operator== (const BlockPosition &position) {
    return (position.x == x && position.y == y && position.z == z);
  }
  
  inline bool operator!= (const BlockPosition &position) {
    return !(position.x == x && position.y == y && position.z == z);
  }
  
  inline BlockPosition area() {
    return create(x & ~(AREASIZE_X-1),y & ~(AREASIZE_Y-1),z & ~(AREASIZE_Z-1));
  }
  
  std::string to_string();
  
  int x;
  int y;
  int z;
};
