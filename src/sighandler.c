#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include "broadcastagent.h"
#include <stdlib.h>

static void CtrlCHandler(int sig){
	signal(sig, SIG_IGN);
	printf("\nStopping server...\n");
    printf("Performing Broadcast Agent Cleanup...\n");
	broadcastAgentCleanup();
    printf("CleanUp finished. Server shut down.");
	exit(0);
}

void CreateCtrlHandler(){
    signal(SIGINT, CtrlCHandler);
}
