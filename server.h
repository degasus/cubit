#include "SDL_net.h"
#include "config.h"


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

