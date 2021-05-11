#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include "util.h"
#include "sighandler.h"
#include "serverconfig.h"
#include "connectionhandler.h"
#include <unistd.h>
#include <errno.h>

int main(int argc, char **argv)
{

	utilInit(argv[0]);
	CreateCtrlHandler();
	debugEnable();
	infoPrint("Chat server, group 5");	
	// https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
	uint16_t socket = (in_port_t)8111;
	char servername[32] = "DEFAULT";
	char adminName[32] = "Admin";
	int c;
	while((c = getopt(argc, argv,"n:p:a:?")) != -1){
		switch (c)
		{
		case 'a': // Admin Name
			strncpy(adminName, optarg, sizeof(adminName) / sizeof(char));
			adminName[31] = '\0';
			break;
		case 'n': // Name
			strncpy(servername, optarg, sizeof(servername) / sizeof(char));
			servername[31] = '\0';
			break;
		case 'p':
			errno = 0;
			debugPrint("GIVEN P: %s",optarg);
			char* ptr = optarg;
			while(*ptr != '\0'){
				if(!isdigit(*ptr)){
					errorPrint("ERR: Given Port is not a number!");
					exit(0);
				}
				ptr++;
			}

			long value = strtol(optarg, NULL, 10);
			if(errno == ERANGE || value > UINT16_MAX || value < 0){
				errorPrint("ERR: Given Port is out of range!");
				exit(1);
			}else if(errno != 0){
				errnoPrint("ERR: Failed to parse given Port.");
				exit(1);
			}
			socket = (in_port_t)value;
			break;
		case '?':
			infoPrint("Following arguments are allowed:\n\n-p (Port as Integer)\n-n (Name as String)");
			exit(0);
		}
	}
	
	serverconfig_init(socket,servername,adminName);
	infoPrint(" Name > %s \n Port > %i\n AdminName > %s",serverconfig_getName(), serverconfig_getPort(), serverconfig_getAdminName());

	const int result = connectionHandler(serverconfig_getPort());
	//TODO: perform cleanup, if required by your implementation
	return result != -1 ? EXIT_SUCCESS : EXIT_FAILURE;
}

