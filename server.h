#include "SDL_net.h"
#include "config.h"

struct Client{
  TCPsocket socket;
  char buffer[64*1024+3];
  int buffer_usage;
  
  
};

