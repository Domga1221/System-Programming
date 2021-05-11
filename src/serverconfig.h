#ifndef SERVERCFG_H
#define SERVERCFG_H
#include <stdint.h>
void serverconfig_init(uint16_t port, char* name, char* adminName);

uint16_t serverconfig_getPort();
int serverconfig_getNameLen();
char* serverconfig_getName();
char* serverconfig_getAdminName();
#endif