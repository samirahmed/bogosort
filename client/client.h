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
  Player* my_player;
} Client;

typedef struct RequestHandler{
  Client * client;
  Proto_Msg_Types type;
  Action_Types action_type;
  Pos current;
  Pos next;
}Request;

typedef struct GlobalsInfo {
  char host[STRLEN];
  PortType port;
}Globals;




//Initilization Functions
void globals_init(int argc, char argv[][STRLEN]);//Had to Change function header to make this work
static int client_init(Client *C);
int client_map_init(Client *C,char* filename);

//RPC calls based on request information
int doRPCCmd(Request* request); //Master RPC CALLER RAWR

//Set Request Types and Required Information
void request_action_init(Request* request, Client* client, Action_Types action, Pos* current, Pos* next);
void request_hello_init(Request* request, Client* client);
void request_goodbye_init(Request* request, Client* client);
void request_sync_init(Request* request, Client* client);

//Read proto_session informations
int process_RPC_message(Client* c);
int process_hello_request(Maze* maze, Player* my_player, Proto_Client_Handle ch, Proto_Msg_Hdr* hdr);
int process_goodbye_request(Proto_Client_Handle ch, Proto_Msg_Hdr* hdr);
int process_action_request(Player* my_player, Proto_Client_Handle ch);
int process_sync_request(Maze* maze, Proto_Client_Handle ch, Proto_Msg_Hdr* hdr);
        
//Updating Maze functions
void update_players(int num_elements,int* player_compress,Maze* maze);
void update_objects(int num_elements,int* object_compress,Maze* maze);
void update_walls(int num_elements,int* game_compress,Maze* maze);

//Connection and Disconnection
int startConnection(Client *C, char *host, PortType port, Proto_MT_Handler h);
void disconnect (Client *C);
int doConnect(Client *C, char* cmd,Request* request);

// Command Line Interface
int docmd(Client *C, char* cmd,Request* request);
void * shell(void *arg);
void usage(char *pgm);
char* prompt(int menu); 

//TODO Implement these?
static int update_handler(Proto_Session *s );
static int update_event_handler(Proto_Session *s);

#endif
