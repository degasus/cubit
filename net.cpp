#include "controller.h"
#include "net.h"

Net::Net(char* server_adress) {
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
	* Sie besteht aus Typ, IP-Adresse und Portnummer */
	memset( &server, 0, sizeof (server));
	if ((addr = inet_addr( server_adress)) != INADDR_NONE) {
		/* argv[1] ist eine numerische IP-Adresse */
		memcpy( (char *)&server.sin_addr, &addr, sizeof(addr));
	}
	else {
		/* Wandle den Servernamen in eine IP-Adresse um */
		host_info = gethostbyname( server_adress);
		if (NULL == host_info) {
			fprintf( stderr, "unknown server: %s\n", server_adress);
			exit(1);
		}
		memcpy( (char *)&server.sin_addr, host_info->h_addr, host_info->h_length);
	}
	
	server.sin_family = AF_INET;
	server.sin_port = htons( PORT);
	
	/* Baue die Verbindung zum Server auf */
	if ( connect( sock, (struct sockaddr*)&server, sizeof( server)) < 0) {
		perror( "can't connect to server");
		exit(1);
	}
}

Net::~Net() {
	closesocket( sock);
}

void Net::changeBlock(BlockPosition block, Material newBlockType) {

}

void Net::loadArea(BlockPosition block) {

}

void Net::moveTo(PlayerPosition pos) {

}

void Net::subscribeToArea(BlockPosition block) {

}

void Net::unsubscribeFromArea(BlockPosition block) {

}

void Net::sendToServer(char* str) {
	send( sock, str, strlen( str), 0);
}

int Net::readFromServer(char* readBuffer[]) {	
	return recv( sock, readBuffer, sizeof(readBuffer), 0);
}