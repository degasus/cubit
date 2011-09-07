#include "SDL_net.h"
#include <list>

#include "server.h"

int main() {
  IPaddress ip;
  TCPsocket tcpsock, client;
  std::list<TCPsocket> clients;

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
    printf("There are %d sockets with activity!\n",numready);

    if(numready>0){
      // check all sockets with SDLNet_SocketReady and handle the active ones.
      if(SDLNet_SocketReady(tcpsock)) {
	client=SDLNet_TCP_Accept(tcpsock);
	if(client) {
	    clients.push_back(client);
	    printf("Added a client to list!\n");
	    numused=SDLNet_TCP_AddSocket(set,client);
	    if(numused==-1) {
		printf("SDLNet_AddSocket: %s\n", SDLNet_GetError());
		// perhaps you need to restart the set and make it bigger...
	    }
	}
      }
      std::list<TCPsocket>::iterator it;
      bool toRemove = false;
      for(it=clients.begin();it!=clients.end();it++){
        if(toRemove)
          clients.remove(client);
        toRemove = false;
	client = *it;
        if(SDLNet_SocketReady(client)) {
          #define MAXLEN 1024
          int result;
          char msg[MAXLEN];
          
          result=SDLNet_TCP_Recv(client,msg,MAXLEN);
          if(result<=0) {
            // An error may have occured, but sometimes you can just ignore it
            // It may be good to disconnect socket because it is likely invalid now.
            numused=SDLNet_TCP_DelSocket(set,client);
            if(numused==-1) {
              printf("SDLNet_DelSocket: %s\n", SDLNet_GetError());
              // perhaps the socket is not in the set
            }
            toRemove = true;
            SDLNet_TCP_Close(client);
          }
          printf("Received: \"%s\"\n",msg);
        }
      }
    }
  }
}