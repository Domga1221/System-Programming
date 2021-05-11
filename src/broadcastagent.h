#ifndef BROADCASTAGENT_H
#define BROADCASTAGENT_H
#include "network.h"

int broadcastAgentInit(void);
void broadcastAgentCleanup(void);

int addToMQueue(Message *msg, int do_lock, int supress_disconnect);
int ba_pause();
int ba_resume();

#endif
