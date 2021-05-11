#ifndef USER_H
#define USER_H

#include <pthread.h>
#include "network.h"

typedef struct User
{
	struct User *prev;
	struct User *next;
	pthread_t thread;	//thread ID of the client thread
	int sock;		//socket for client
	char name[32];  // null terminated
} User;


User *add_user(int socket,pthread_t thread, char *uname, int do_lock);
void all_users(void(*func)(User *));
int remove_user(User * user, int do_lock);
User *getFront();
User *getUser(const char *username);
int checkName(const char *name);
int kick(const char *username);


int kick(const char *userName);

void lockMutex();
void unlockMutex();
#endif
