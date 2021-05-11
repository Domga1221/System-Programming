#include "serverconfig.h"
#include <stdlib.h>
#include "util.h"
#include <string.h>
#include <stdio.h>

static char name[32];
static char adminName[32] = "Brot";
static uint16_t port;
static int isInit = 0;

static int getStringLen(char* s){
    if(s == NULL){
        return -1;
    }
    int counter = 0;
    while(s[counter] != '\0'){
        counter++;
    }
    return counter;
}

void serverconfig_init(uint16_t _port, char* _name, char* _admin){
    if(isInit){
        debugPrint("WARN: Serverconfig has already been initialized. The call to initialize has been ignored.");
        return;
    }

    strncpy(adminName, _admin, 32);
    strncpy(name, _name, 32);
    adminName[31] = name[31] = '\0';
    
    port = (_port > 0) ? _port : 8111;
    isInit = 1;
}

int serverconfig_getNameLen(){
    char* namePtr = name;
    return getStringLen(namePtr);
}

char* serverconfig_getName(){
    char* namePtr = name;
    return namePtr;
}

uint16_t serverconfig_getPort(){
    return port;
}

char* serverconfig_getAdminName(){
    return adminName;
}