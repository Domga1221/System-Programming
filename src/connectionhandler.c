#include <errno.h>
#include "connectionhandler.h"
#include "util.h"
#include "user.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clientthread.h"

#define QUEUE_LEN 100

struct sockaddr_in localAddress; 

static int UpdateLocalhostAddressInfo(in_port_t port){
	memset(&localAddress,0,sizeof(localAddress));
	localAddress.sin_family = AF_INET;
	localAddress.sin_port = htons(port);
	localAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	return 0;
}



static int createPassiveSocket(in_port_t port)
{
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock == -1){
		errorPrint("Could not create socket.");
		errno = ESOCKTNOSUPPORT;
		return -1;
	}
	
	UpdateLocalhostAddressInfo(port);
	
	if(bind(sock, (const struct sockaddr*)&localAddress,sizeof(localAddress)) < 0)
	{
		//Error
		errorPrint("Could not bind socket to address.");
		
		return -1;
	}
	debugPrint("%i",sock);
	const int backlogSize = 10;
	int listenResponse = listen(sock, backlogSize);
	if(listenResponse == 0){
		return sock;
	}else{
		errorPrint("Could not listen on socket. ErrorCode: %i",errno);
		return -1;

	}
}


int connectionHandler(in_port_t port)
{
	

	const int fd = createPassiveSocket(port);
	if(fd == -1)
	{
		errnoPrint("Unable to create server socket");
		return -1;
	}

	infoPrint("Passive Socket Created on Port %d",port);	
	
	if(listen(fd, QUEUE_LEN) < 0){
		errnoPrint("Unable to listen on Port");
	}
	infoPrint("Listening on %#010x",localAddress.sin_addr.s_addr);

	while(true)
	{
		
		
		//int sockDescriptor = accept(fd,localAddress->ai_addr->sa_data,(socklen_t)(localAddress->ai_addrlen));
		//int sockDescriptor = accept(fd,NULL,NULL);

		const int sockDescriptor = accept(fd,NULL,NULL);
		if(sockDescriptor < 0){
			errnoPrint("SocketDescriptor ist %i",sockDescriptor);
		}
		infoPrint("Accepting Connection, Sock Descriptor: %d",sockDescriptor);
		
		User* user = malloc(sizeof(User));
		memset(user, 0, sizeof(User));
		user->sock = sockDescriptor;
		
		pthread_create(&(user->thread),NULL,&clientthread,user);
		
		//TODO: add connection to user list and start client thread
		infoPrint("%li",user->thread);
		
	}

	return 0;	//never reached
}