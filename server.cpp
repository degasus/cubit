#include "server.h"

static void serv_request( int in, int out, char* rootpath);

Server::Server() {
	struct sockaddr_in server, client;
	int sock, fd;
	int len;
	
	#ifdef _WIN32  
	/* Initialisiere TCP für Windows ("winsock") */
	short wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD (1, 1);
	if (WSAStartup (wVersionRequested, &wsaData) != 0) {
		fprintf( stderr, "Failed to init windows sockets\n");
		exit(1);
	}
	#endif
	
	/* Erzeuge das Socket */
	sock = socket( PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror( "failed to create socket");
		exit(1);
	}
	
	/* Erzeuge die Socketadresse des Servers 
	* Sie besteht aus Typ und Portnummer */
	memset( &server, 0, sizeof (server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl( INADDR_ANY);
	server.sin_port = htons( PORT);
	
	/* Erzeuge die Bindung an die Serveradresse 
	* (d.h. an einen bestimmten Port) */
	if (bind( sock, (struct sockaddr*)&server, sizeof( server)) < 0) {
		perror( "can't bind socket");
		exit(1);
	}
	
	/* Teile dem Socket mit, dass Verbindungswünsche
	* von Clients entgegengenommen werden */
	listen( sock, 5);
}

int main( int argc, char **argv)
{
	Server::Server() *s = new Server::Server();
	s->run();
	delete s;
	return 0;
}
	
/*
* serv_request
* Bearbeite den auf in ankommenden http request
* Die zu sendenden Daten werden auf out ausgegeben
*/
static void serv_request( int in, int out, char* rootpath)
{
	char buffer[8192];
	char *b, *l, *le;
	int count, totalcount;
	char url[256];
	char path[256];
	int fd;
	int eoh = 0;
	
	b = buffer;
	l = buffer;
	totalcount = 0;
	*url = 0;
	while ( (count = recv( in, b, sizeof(buffer) - totalcount, 0)) > 0) {
		totalcount += count;
		b += count;
		while (l < b) {
			le = l;
			while (le < b && *le != '\n' && *le != '\r') ++le;
			if ('\n' == *le || '\r' == *le) {
				*le = 0;
				printf ("Header line = "%s"\n", l);
				sscanf( l, "GET %255s HTTP/", url);
				if (strlen(l)) eoh = 1;
				l = le + 1;
			}
		}
		if (eoh) break;
	}
	
	if ( strlen(url)) {
		printf( "got request: GET %s\n", url);
		sprintf(path, "%s/%s", rootpath, url);
		fd = open( path, O_RDONLY);
		if (fd > 0) {
			sprintf( buffer, "HTTP/1.0 200 OK\nContent-Type: text/html\n\n");
			send( out, buffer, strlen(buffer), 0);
			do {
				count = read( fd, buffer, sizeof(buffer));
				send( out, buffer, count, 0);
				printf(".");
				fflush(stdout);
			} while (count > 0);
			close( fd);
			printf("finished request: GET %s\n", url);
		}
		else {
			sprintf( buffer, "HTTP/1.0 404 Not Found\n\n");
			send( out, buffer, strlen(buffer), 0);
		}
	}
	else {
		sprintf( buffer, "HTTP/1.0 501 Method Not Implemented\n\n");
		send( out, buffer, strlen(buffer), 0);
	}
}