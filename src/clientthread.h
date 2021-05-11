#ifndef CLIENTTHREAD_H
#define CLIENTTHREAD_H

#include "user.h"
#include "network.h"

void *clientthread(void *arg);
void disconnectedUserRemove(User *self, int code, int do_lock);
#endif
