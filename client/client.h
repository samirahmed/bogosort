#ifndef __DAGAME_MYCLIENT_H__
#define __DAGAME_MYCLIENT_H__

//Global Variables
static int connected;
static char MenuString[] = "\n?> ";

struct Request{
  Client * client;
  Proto_Msg_Types type;
  int x;
  int y;
  Team_Types turf;
  Cell_Types cell_type;
} request;

struct Globals {
  char host[STRLEN];
  PortType port;
} globals;


// Client State typedef
typedef struct ClientState  {
  int data;
  Proto_Client_Handle ph;
  Maze maze;
  Blocking_Helper bh;
} Client;


//Initilization Functions
void initGlobals(int argc, char argv[][STRLEN]);//Had to Change function header to make this work
static int clientInit(Client *C)
int init_client_map(Client *C,char* filename)

//RPC calls
int doRPCCmd() 

//Connection and Disconnection
int startConnection(Client *C, char *host, PortType port, Proto_MT_Handler h)
void disconnect (Client *C)
int doConnect(Client *C, char* cmd)

// Command Line Interface
int docmd(Client *C, char* cmd)
void * shell(void *arg)
void usage(char *pgm)
char* prompt(int menu) 

//TODO Implement these?
static int update_handler(Proto_Session *s )
static int update_event_handler(Proto_Session *s)




#endif
