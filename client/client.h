#ifndef __DAGAME_MYCLIENT_H__
#define __DAGAME_MYCLIENT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "client.h"
#include "../lib/types.h"
#include "../lib/protocol.h"
#include "../lib/protocol_client.h"
#include "../lib/protocol_utils.h"
#include "../lib/game_client.h"

#define STRLEN 81

typedef struct ClientState  {
  Proto_Client_Handle ph;
  Maze maze;
  Blocking_Helper bh;
} Client;

typedef struct RequestHandler{
  Client * client;
  Proto_Msg_Types type;
  int x;
  int y;
}Request;

typedef struct GlobalsInfo {
  char host[STRLEN];
  PortType port;
}Globals;




//Initilization Functions
void globals_init(int argc, char argv[][STRLEN]);//Had to Change function header to make this work
static int client_init(Client *C);
int client_map_init(Client *C,char* filename);

//RPC calls
int doRPCCmd(Request* request); //Master RPC CALLER RAWR

//Set Request Header and call doRPCCmd()
int send_hello_rpc(Request* request);
int send_action_rpc(Request* request);
int send_sync_rpc(Request* request);
int send_goodbye_rpc(Request* request);




//Connection and Disconnection
int startConnection(Client *C, char *host, PortType port, Proto_MT_Handler h);
void disconnect (Client *C);
int doConnect(Client *C, char* cmd);

// Command Line Interface
int docmd(Client *C, char* cmd);
void * shell(void *arg);
void usage(char *pgm);
char* prompt(int menu); 

//TODO Implement these?
static int update_handler(Proto_Session *s );
static int update_event_handler(Proto_Session *s);

#endif
