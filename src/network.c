#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include "network.h"
#include "util.h"
#include <assert.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include "network.datahandler.h"
#include "serverconfig.h"
#include <errno.h>
#include <stddef.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>


int networkReceive(int fd, Message *buffer)
{
	debugPrint("Entering networkReceive, fd: %i", fd);


	// Receive Type and Length
	int ret = recv(fd, buffer, sizeof(buffer->header.type) + sizeof(buffer->header.length), MSG_WAITALL);
	
	if(ret <= 0){
		errnoPrint("Error on receive from fd: %i, returns %i", fd, ret);
		return ret;
	}

	uint16_t length = ntohs(buffer->header.length);
	if(length > MSG_MAX){
		errorPrint("Message too big");
		return -1;
	}

	// Receive actual message, either an LRQ or a C2S-message
	if(buffer->header.type == LOGIN_REQUEST){
		ret = recv(fd, &(buffer->body.LRQ), length, MSG_WAITALL);
	} else{
		ret = recv(fd, &buffer->body.C2S.message, length, MSG_WAITALL);
	}
	return ret;
}

int networkSend(int fd, const Message *buffer)
{
	debugPrint("Entering networkSend, fd: %i", fd);
	size_t trueTransmitLen = ntohs(buffer->header.length) + 3; // +3 sind die Bytes des Headers
	//Debug
	void* buf = (void*) buffer;
	//debugHexdump(buf, trueTransmitLen, NULL);
	
	if(send(fd,(void*)(buf),trueTransmitLen,MSG_NOSIGNAL) == -1){
		errorPrint("Konnte Nachricht nicht absenden. Socket Descriptor: %d, Nachricht: %s",fd,buffer->body.C2S.message);
		return -1;
	}
	debugPrint("Message successfully sent, Exiting networkSend, fd: %i ", fd);
	return 0;
}

Message createLoginResponse(uint8_t code){
	Message msg;
	msg.header.type = 1; 
	msg.body.LRE.magic = htonl(LRE_MAGIC);
	msg.body.LRE.code = code;
	strcpy(msg.body.LRE.serverName, serverconfig_getName());
	msg.header.length = htons(5 + strlen(msg.body.LRE.serverName));
	return msg;
}

Message createUserAdded(const char *name, uint64_t timeStamp){
	Message msg;
	msg.header.type = 4;
	int nameLen = strlen(name);
	msg.header.length = htons(8 + nameLen);
	strncpy(msg.body.UAD.userName, name,31);

	msg.body.UAD.timeStamp = hton64u(timeStamp);
	return msg;
}

Message createUserRemoved(const char *name, uint8_t code){
	Message msg;
	msg.header.type = 5;
	int nameLen = strlen(name);
	msg.header.length = htons(9 + nameLen);
	strncpy(msg.body.URM.userName, name,31);
	msg.body.URM.timeStamp = hton64u(time(NULL));
	msg.body.URM.code = code;
	return msg;
}

Message createS2CfromC2S(Message c2s, const char *name){
	char text[512];
	memset(text, 0, sizeof(text));
	strncpy(text, c2s.body.C2S.message, ntohs(c2s.header.length));
	strcpy(c2s.body.S2C.message, text);
	strcpy(c2s.body.S2C.originalSender, name);
	c2s.header.length = htons(40 + strlen(text));
	c2s.body.S2C.timeStamp = hton64u(time(NULL));
	return c2s;
}

Message createServerMessage(const char *errorMsg){
	Message msg;
	msg.header.type = 3;
	strcpy(msg.body.S2C.message, errorMsg);
	msg.body.S2C.originalSender[0] = '\0';
	msg.header.length = htons(40 + strlen(errorMsg));
	msg.body.S2C.timeStamp = hton64u(time(NULL));
	return msg; 
}

Message createPauseMessage(const char *text){
	Message msg;
	char data[512];
	strcpy(data, serverconfig_getAdminName());
	strcpy(&data[strlen(serverconfig_getAdminName())], " ");
	strcpy(&data[strlen(serverconfig_getAdminName()) + 1], text);
	msg = createServerMessage(data);
	return msg;
}




