#ifndef CHAT_PROTOCOL_H
#define CHAT_PROTOCOL_H
#include <stdint.h>

/* TODO: When implementing the fully-featured network protocol (including
 * login), replace this with message structures derived from the network
 * protocol (RFC) as found in the moodle course. */


enum { MSG_MAX = 1024 };

// Enums
typedef enum {
	LOGIN_REQUEST,
	LOGIN_RESPONSE,
	CLIENT_2_Server,
	SERVER_2_CLIENT,
	USER_ADDED,
	USER_REMOVED
}MessageTypes;

typedef enum {
	LRQ_MAGIC = 0x0badf00d,
	LRE_MAGIC = 0xc001c001
}MagicNumbers;

typedef enum LoginResponseStatus{
	LRE_SUCCESS = 0,
	LRE_NAMETAKEN = 1,
	LRE_NAMEINVALID = 2,
	LRE_WRONGVERSION = 3,
	LRE_UNKNOWNERROR = 255,
} LoginResponseStatus;

typedef enum UserRemovedStatus{
	URM_CLOSEDBYCLIENT = 0,
	URM_KICKED = 1,
	URM_COMMUNICATIONERR = 2,
} UserRemovedStatus;

typedef enum {
	LRQ_MIN = 5,
	LRE_MIN = 5,
	C2S_MIN = 0,
	S2C_MIN = 40,
	UAD_MIN = 8,
	URM_MIN = 9
}MinLengths;


// Message structures - no Padding
#pragma pack(1)

typedef struct LoginRequest{
	uint32_t magic;
	uint8_t version;
	char userName[31];
} LoginRequest;


typedef struct LoginResponse{
	uint32_t magic;
	uint8_t code;
	char serverName[31];
} LoginResponse;

typedef struct Client2Server{
	char message[512];
} Client2Server;

typedef struct Server2Client{
	uint64_t timeStamp;
	char originalSender[32];
	char message[512];
} Server2Client;

typedef struct UserAdded{
	uint64_t timeStamp;
	char userName[31];
} UserAdded;

typedef struct UserRemoved{
	uint64_t timeStamp;
	uint8_t code;
	char userName[31];
} UserRemoved;

typedef union{
	LoginRequest LRQ;
	LoginResponse LRE;
	Client2Server C2S;
	Server2Client S2C;
	UserAdded UAD;
	UserRemoved URM;
}Body;

typedef struct{
	uint8_t type;
	uint16_t length;
}Header;

typedef struct Message{
	Header header;
	Body body;
}Message;

#pragma pack(0)


// Funktionen
int networkReceive(int fd, Message *buffer);
int networkSend(int fd, const Message *buffer);

Message createLoginResponse(uint8_t code);
Message createUserAdded(const char *name, uint64_t timeStamp);
Message createUserRemoved(const char *name, uint8_t code);

Message createS2CfromC2S(Message c2s, const char *name);

Message createServerMessage(const char *errorMsg);
Message createPauseMessage(const char *text);



#endif
