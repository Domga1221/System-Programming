#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>
#include "broadcastagent.h"
#include "util.h"
#include <sys/stat.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include "network.h"
#include <signal.h>
#include <unistd.h>
#include "clientthread.h"
#include <sys/prctl.h>
#include "user.h"
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include "sighandler.h"

mqd_t messageQueue = 0;
pthread_t threadId = 0;

sem_t sem;
int semInit = 0;
int isInit = 0;
void sendall(Message msg, int do_lock, int supress_disconnect);
void ba_wait();
void ba_post();
int isQueueFull();
int paused = 0;



void *broadcastAgent(void *arg){
	CreateCtrlHandler();
	Message msg;
	while(1){
		if(paused){
		// Queue is paused. Wait a bit.
			struct timespec ms;
			ms.tv_sec = 0;
			ms.tv_nsec = 10000000L;	// 10ms
			nanosleep(&ms, NULL);
			continue;
		}
		int ret = mq_receive(messageQueue, (char*) &msg, sizeof(msg), NULL);
		
		if(ret < 0){
			if(errno == EAGAIN){
				// Queue is empty. Wait a bit.
				struct timespec ms;
				ms.tv_sec = 0;
				ms.tv_nsec = 10000000L;	// 10ms
				nanosleep(&ms, NULL);
			}
			else{
				//unknown error:
				assert(0);
			}
		}else
		{
			ba_wait();
			sendall(msg, 1, 0);
			ba_post();
			/* code */
		}
		
		
	}
	return arg;
}

int broadcastAgentInit(void){
	if(isInit){
		return 0;
	}
	mq_unlink("/bq");
	if(messageQueue == 0){
		struct mq_attr attributes;
		attributes.mq_flags = O_NONBLOCK;
		attributes.mq_maxmsg = 10;
		attributes.mq_msgsize = sizeof(Message);
		attributes.mq_curmsgs = 0;
		messageQueue = mq_open("/bq", O_CREAT | O_RDWR, S_IRWXU, &attributes);
		if(messageQueue == (mqd_t) -1){
			perror("Could not open Message Queue");
			return -1;
		}
	}

	if(threadId == 0){
		if(pthread_create(&threadId, NULL, broadcastAgent, NULL)){
			perror("Could not create thread for broadcast");
			return -1;
		}
		//prctl(PR_SET_NAME, "BROADCAST");
		//pthread_setname_np(threadId, "BROADCAST");
	}

	if(semInit == 0){
		if(sem_init(&sem, 0, 1) < 0){
			return -1;
		}
		semInit = 1;
	}
	isInit = 1;
	return 0;
}

void broadcastAgentCleanup(){
	if(messageQueue != 0){
		mq_close(messageQueue);
		mq_unlink("/bq");
	}

	if(semInit){
		sem_destroy(&sem);
	}

	if(threadId != 0){
		pthread_cancel(threadId);
	}
}

int addToMQueue(Message *msg, int do_lock, int supress_disconnect){
	debugPrint("Entering addToMQueue()");
	int returnValue = 0;
	if(msg->header.type == SERVER_2_CLIENT){
		if(isQueueFull()){	
			debugPrint("Queue is full. Waiting...");
			return -1;
		}
		
		returnValue = mq_send(messageQueue, (char*)msg, ntohs(msg->header.length) + 3, 0);
		
	} else{
		debugPrint("Not S2C. Sent, mQueue is not being used.");
		sendall(*msg, do_lock, supress_disconnect);
		debugPrint("after sendall");
		return 0;
	}
	struct mq_attr attrs; 
	mq_getattr(messageQueue, &attrs);
	debugPrint("QueueLen > %li/%li", attrs.mq_curmsgs, attrs.mq_maxmsg);
	return returnValue;
}


void sendall(Message msg, int do_lock, int supress_disconnect){
	
	if(do_lock){
		lockMutex();
	}

	User *user = getFront();
	while(user != NULL){
		assert(user->name[0] != '\0');
		
		if(networkSend(user->sock, &msg) < 0){
			if(errno == EPIPE && !supress_disconnect){
				//Broken Pipe, disconnect user
				disconnectedUserRemove(user, URM_COMMUNICATIONERR, do_lock);
			}
		}
		
		user = user->next;
	}

	if(do_lock){
		unlockMutex();
	}
}

int ba_pause(){
	if(paused){
		return -1;
	}

	ba_wait();
	paused = 1;
	sendall(createPauseMessage("has paused the chat."), 1, 0);
	return 0;
}	

void ba_wait(){
	sem_wait(&sem);
}

int ba_resume(){
	if(paused){
		ba_post();
		paused = 0;
		sendall(createPauseMessage("has resumed the chat."), 1, 0);
		return 0;
	}
	return -1;
}

void ba_post(){
	sem_post(&sem);
}

int isQueueFull(){
	struct mq_attr attrs; 
	mq_getattr(messageQueue, &attrs);
	return attrs.mq_curmsgs >= attrs.mq_maxmsg;
}