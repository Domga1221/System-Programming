#include <netinet/in.h>
#include <assert.h>
#include <stdlib.h>
#include "clientthread.h"
#include "network.datahandler.h"
#include "user.h"
#include "util.h"
#include <sys/prctl.h>
#include <time.h>
#include "network.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "broadcastagent.h"
#include "serverconfig.h"
#include <unistd.h>
#include <signal.h>
#include "sighandler.h"
static int interact(User *self);
static void handleC2S(User *self, Message*msg);
void HandleCommand(char* command, User* sender);
static int checkLRQ(Message *msg);
static int checkingInfo(User *self, Message *msg, char* name);
static int checkingSelf(User *user, Message *msg);
static void sendUserAddedMessages(User *self, Message *msg);



void *clientthread(void* arg)
{
	CreateCtrlHandler();
	User* self = (User*)arg;
	debugPrint("Client Thread Started");
	debugPrint("Thread-Id: %li",pthread_self());
	Message msg;
	memset(&msg, 0, sizeof(Message));
	networkReceive(self->sock, &msg);
	self->thread = pthread_self();
	char name[32];
 	if(msg.header.type == 0 && ntohl(msg.body.LRQ.magic) == LRQ_MAGIC){
		uint16_t msgLen = ntohs(msg.header.length);
		strncpy(name, msg.body.LRQ.userName, msgLen - 5);
		name[msgLen - 5] = '\0';	
		strcpy(self->name, name);
	}
	else{
		Message response = createLoginResponse(LRE_UNKNOWNERROR);
		networkSend(self->sock, &response);
		return NULL;
	}


	if(checkingInfo(self, &msg, name) < 0){
		return NULL;
	}
	debugPrint("bla1");

	if(checkingSelf(self, &msg) < 0){
		return NULL;
	}

	debugPrint("bla2");

	Message LoginSuccess = createLoginResponse(LRE_SUCCESS);
	networkSend(self->sock, &LoginSuccess);
	lockMutex();
	self = add_user(self->sock,self->thread, self->name, 0);
	
	// Create User Added, send to all users
	Message UserAddedMessage = createUserAdded(self->name, time(NULL));
	sendUserAddedMessages(self, &UserAddedMessage);
	unlockMutex();

	// Send to self, confirming correct login
	networkSend(self->sock, &UserAddedMessage);
	broadcastAgentInit();
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	if(interact(self) < 0){
		return NULL;
	}

	debugPrint("Client thread Stopping");
	return NULL;
}

// static: only visible in object file
static int interact(User *self){
	debugPrint("Entering interact()");

	while(1){
		Message msg;
		memset(&msg, 0, sizeof(msg));
		errno = 0;
		debugPrint("Waiting, %s", self->name);
		int ret = networkReceive(self->sock, &msg);
		// Did client close connection?
		if(ret == 0 || errno == ECONNRESET){	
			debugPrint("Client closed connection");
			disconnectedUserRemove(self, URM_CLOSEDBYCLIENT, 1);
			return -1;
			
		} else if(ret == -1){
			return -1;
		}
		else{
			handleC2S(self, &msg);
		}
	}
}

static int checkingInfo(User *self, Message *msg, char* name){
	debugPrint("Check if user can still receive data, UserPtr: %li, MsgPtr: %li", (long)self, (long)msg);


	// validate Protocol
	if(checkLRQ(msg) < 0){
		if(errno == EPROTONOSUPPORT){
		*msg = createLoginResponse(LRE_WRONGVERSION); // 3: Protocol Version mismatch
		networkSend(self->sock, msg);
		debugPrint("Thread stopping due to missmatch in Protocol");
		}
		else if (errno == ENOMSG){
			*msg = createLoginResponse(LRE_UNKNOWNERROR); // 255: Unknown Err
			networkSend(self->sock, msg);
			debugPrint("Thread stopping due to missmatch in Protocol");
		}
		debugPrint("Client thread stopping");
		return -1;
	}
	name[31] = '\0';

	// validate userName
	if(checkName(name) < 0){
		Message errmsg = createLoginResponse(LRE_NAMEINVALID); // 2: Name invalid
		
		networkSend(self->sock, &errmsg);
		debugPrint("Client thread stopping");
		return -1;
	}

	// user Name in use?
	if(getUser(name) != NULL){
		// Name in use
		debugPrint("Name already in use");
		Message errMsg = createLoginResponse(LRE_NAMETAKEN);
		networkSend(self->sock, &errMsg);
		return -1;
	}

	return 0;
}

static int checkingSelf(User *user, Message *msg){
	if(user == NULL){
		errorPrint("User is NULL");
		*msg = createLoginResponse(LRE_UNKNOWNERROR);
		networkSend(user->sock, msg);
		close(user->sock); // close socket
		debugPrint("Client Thread terminated");
		return -1;
	}
	return 0;
}

static void sendUserAddedMessages(User *self, Message *msg){
	//lockMutex();
	User *user = getFront();
	while(user != NULL){
		if(user == self){
			user = user->next;
			continue;
		}
		assert(user->name[0] != '\0');
		char userName[32];
		strcpy(userName, user->name);

		//create User Added Message for existing clients to be passed to new client
		Message ForNewClient = createUserAdded(userName, 0);
		networkSend(self->sock, &ForNewClient);

		// create USer Added Message for new client to be passed to existing clients
		networkSend(user->sock, msg);

		user = user->next;
	}
	//unlockMutex();
}

void disconnectedUserRemove(User *self, int code, int do_lock){
	debugPrint("Entering disconnectedUserRemove()");
	Message URM = createUserRemoved(self->name, code);
	if(do_lock){
		lockMutex();
	}
	addToMQueue(&URM, 0, 1);
	debugPrint("Removing user: %s", self->name);
	remove_user(self, 0);
	if(do_lock){
	unlockMutex();
	}
	debugPrint("User removed");
	pthread_cancel(self->thread);
	debugPrint("Thread terminated");
}

static void handleC2S(User *self, Message *msg){
	char text[512];
	memset(text, '\0', 512);
	strcpy(text, (msg->body).C2S.message);
	(msg->header).type = 3; // Server to Client
	
	if(text[0] == '/'){ // Slash
		HandleCommand(text, self);
	} else{
		char name[32];
		memset(name, '\0',32);
		strcpy(name, self->name);
		Message response = createS2CfromC2S(*msg, name);
		if(addToMQueue(&response, 1, 0) < 0){
			response = createServerMessage("Message queue is full");
			networkSend(self->sock, &response);
		}
	}
}

static int checkLRQ(Message *msg){

	if(ntohl(msg->body.LRQ.magic) != LRQ_MAGIC){
		errno = ENOMSG;
		return -1;
	}	

	if(msg->header.type != 0){
		errno = ENOMSG;
		return -1;
	}

	if(ntohs(msg->header.length) < 6 || ntohs(msg->header.length) > 36){
		errno = ENOMSG;
		return -1;
	}

	if(msg->body.LRQ.version != 0){
		errno = EPROTONOSUPPORT;
		return -1;
	}
	return 0;
}

void HandleCommand(char* command, User* sender){
    Message msg;
    if(strcmp(serverconfig_getAdminName(), sender->name) == 0){
        if(strncmp(command,"/kick",5) == 0){
            char userName[32];
            strncpy(userName, &command[6], 32);  
            int a = kick(userName);
            if(a < 0){
                if(errno == EILSEQ){
                    msg = createServerMessage("Can't kick yourself");
                } else if(errno == ENOENT){
                    msg = createServerMessage("User does not exist");
                } else{
                    msg = createServerMessage("Unknown error when kicking user");
                }
                networkSend(sender->sock, &msg);
            } else{
                msg = createUserRemoved(userName, URM_KICKED);
                addToMQueue(&msg, 1, 0);
            }
        } else if(strncmp(command,"/pause",6) == 0){
            if(ba_pause() < 0){
                msg = createServerMessage("Server already paused. Ignoring command.");
				networkSend(sender->sock, &msg);
			}
        } else if(strncmp(command,"/resume",7) == 0){
            if(ba_resume() < 0){
                msg = createServerMessage("Server not paused. Ignoring command.");
                networkSend(sender->sock, &msg);
            }
        } else if (strncmp(command, "/help",5) == 0){
			msg = createServerMessage("> /kick [User] - Kick a user\n> /pause - Pause Chat\n> /resume - Resume Chat");
			networkSend(sender->sock, &msg);
		} 
		
		else{
            msg = createServerMessage("Command unknown. use /help to get all commands");
            networkSend(sender->sock, &msg);
        }
    } else{
        msg = createServerMessage("You are not authorized");
        networkSend(sender->sock, &msg);
    }
}