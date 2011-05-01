#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#ifdef _WIN32
/* Headerfiles für Windows */
#include <winsock.h>
#include <io.h>

#else
/* Headerfiles für Unix/Linux */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket(s) close(s)
#endif

#define PORT 1337

class Server {
	public:
		Server();
		~Server();
		
	private:
		run();
};