/******************************************************************************
* Copyright (C) 2011 by Jonathan Appavoo, Boston University
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/types.h"
#include "../lib/protocol_client.h"
#include "../lib/protocol_utils.h"

#define STRLEN 81

//Function Headers
void initGlobals(int argc, char argv[][STRLEN]);//Had to Change function header to make this work

struct Globals {
  char host[STRLEN];
  PortType port;
} globals;

// An Easy Struct for Printing
typedef struct {
	char pos[9];
	int move;
	int tie;
	int over;
	int started;
	int iwin;
	int error;
	int timestamp;
} Board; 

typedef struct ClientState  {
  int data;
  Proto_Client_Handle ph;
} Client;

static int connected;
static int playerid;
static char MenuString[] = "\n?> ";
static Board gameboard;



void board_print(Board *b)
{
	if ( !b->started )
	{
		printf( "\nGame not started\n");
		return;
	}

	printf("\n");
	printf(" %c | %c | %c \n", b->pos[0] , b->pos[1] , b->pos[2] );
	printf("---|---|--- \n");
	printf(" %c | %c | %c \n", b->pos[3] , b->pos[4] , b->pos[5] );
	printf("---|---|--- \n");
	printf(" %c | %c | %c \n", b->pos[6] , b->pos[7] , b->pos[8] );

	if ( b->over )
	{
	 	printf("\n Game Over - ");

		if ( b->iwin ) printf(" You Win ");
		else if (b->tie )printf( "Tie Game");
		else if (b->error)
		connected ? printf( "Opponent Quit - Restart Game") : printf( "Lost Connection\n");
		else printf(" You Lost Sorry ");
	}
	else
	{
		if ( b->move ) printf("\n Your Move");
		else printf("\n Waiting for other player");
	}
}

void board_init(Board *b, Proto_Msg_Hdr *h)
{
	bzero(b, sizeof(Board));
  	bzero(&gameboard, sizeof(Board));

	int ii;
	
	for( ii =0 ; ii < 9; ii++ ) b->pos[ii] = (char)( ii + 48); 

	int player = h->pstate.v0.raw;
	int mask = 1;
	
	for( ii =0 ; ii < 9; ii++ )
	{
		if ( (player & mask)) b->pos[ii]='X';
		player = player >> 1;
	}

	player = h->pstate.v1.raw;
	for( ii =0 ; ii < 9; ii++ )
	{
		if ( (player & mask)) b->pos[ii]='O';
		player = player >> 1;
	}
	
	b->move = 0;
	b->started = 1;
	b->over = 0;
	b->iwin = 0;
	b->tie = 0;
	b->error =0;
	switch ( h->gstate.v0.raw )
	{
		case 0:
			b->started = 0;
			break;
		case 1: 
			b->move = (1 == playerid );
			break;
		case 2: 
			b->move = (2 == playerid );
			break;
		case 3:
			b->iwin = (1 ==playerid);
			b->over = 1;
			break;
		case 4:
			b->iwin = (2 ==playerid);
			b->over = 1;
			break;
		case 5:
			b->over = 1;
			b->tie = 1;
			break;
		case 6:
			b->over = 1;
			b->error = 1;
		default:
			break;
	}
}

int update_handler(Proto_Session *s )
{
	bzero(&gameboard, sizeof(Board));
	Proto_Msg_Hdr h;
	proto_session_hdr_unmarshall(s,&h);
	board_init(&gameboard,&h);
	board_print(&gameboard);
	return 1;
}

static int
clientInit(Client *C)
{
  // Zero global scope
  bzero(C, sizeof(Client));
  bzero(&gameboard, sizeof(Board));
  playerid = 0;
  connected = 0;
  
  // initialize the client protocol subsystem
  if (proto_client_init(&(C->ph))<0) {
    fprintf(stderr, "client: main: ERROR initializing proto system\n");
    return -1;
  }
  
  proto_client_set_event_handler(C->ph, PROTO_MT_EVENT_BASE_UPDATE, update_handler );
  return 1;
}


static int
update_event_handler(Proto_Session *s)
{
  Client *C = proto_session_get_data(s);

  fprintf(stderr, "%s: called", __func__);
  return 1;
}


int 
startConnection(Client *C, char *host, PortType port, Proto_MT_Handler h)
{
  if (globals.host[0]!=0 && globals.port!=0) {
    int res;
	if (( res = proto_client_connect(C->ph, host, port))!=0) {
      fprintf(stderr, "failed to connect\n : Error code %d",res);
      return -1;
    }
    proto_session_set_data(proto_client_event_session(C->ph), C);
#if 0
    if (h != NULL) {
      proto_client_set_event_handler(C->ph, PROTO_MT_EVENT_BASE_UPDATE, 
				     h);
    }
#endif
    return 1;
  }
  return 0;
}

char*
prompt(int menu) 
{
  if (menu) printf("%s", MenuString);
  fflush(stdout);
  
  // Pull in input from stdin
  int bytes_read;
  int nbytes = 0;
  char *my_string;
  bytes_read = getline (&my_string, &nbytes, stdin);

  if(bytes_read>0)
	  return my_string;
  else
	  return 0;

}

int 
game_process_move(Client *C)
{
	int data;
	Proto_Session *s;
	s = proto_client_rpc_session(C->ph);
	proto_session_body_unmarshall_int(s,0,&data);
	if (proto_debug()) fprintf(stderr,"Move response : %d",data);
    if (data == 0xdeadbeef) printf("Server Ignored Request \n");
	if (data == 0) printf("Not Your Move! Wait Your Turn \n");
	if (data < 0) printf("Invalid Move! Try Again \n");
	if (data > 0) printf("Move Accepted!\n");
	return 1;
}

void 
set_player(int ii)
{
	switch (ii)
	{
		case 1:
			playerid = 1;
			MenuString[1] = 'X';
			break;
		case 2:
			playerid = 2;	
			MenuString[1]='O';
			break;
		default:
			playerid = 0;	
			MenuString[1]='?';
			break;
	}
}

int
game_process_hello(Client *C, int rc)
{
  Proto_Session *s;
  
  switch ( rc )
  {
  	case 1:
		set_player(1);
		printf("Connected to %s:%d : You are X", globals.host , (int) globals.port);
		break;
	case 2:
		set_player(2);
		printf("Connected to %s:%d : You are O", globals.host , (int) globals.port);
		break;
	default:
		set_player(0);
		printf("Unable to connect to game");
		break;
  }
  return 1;
}

int 
doRPCCmd(Client *C, char c,char move) 
{
  int rc=-1;

  switch (c) {
  case 'h':  
    {
      rc = proto_client_hello(C->ph);
      if (proto_debug()) fprintf(stderr,"hello: rc=%x\n", rc);
	  if (rc > 0 && playerid == 0)game_process_hello(C,rc); //only process the hello if playerid has not been set
	  else fprintf(stderr, "Your playerid has already been set. Your playerid is: %d\n", playerid);
    }
    break;
  case 'm':
    rc = proto_client_move(C->ph, playerid, move);
	if (rc > 0) game_process_move(C);
    break;
  case 'g':
    rc = proto_client_goodbye(C->ph);
    printf("Game Over - You Quit");
	printf("RC = %d",rc);
	break;
  default:
    printf("%s: unknown command %c\n", __func__, c);
  }
  // NULL MT OVERRIDE ;-)
  if(proto_debug()) fprintf(stderr,"%s: rc=0x%x\n", __func__, rc);
  if (rc == 0xdeadbeef) rc=1;
  return rc;
}

int
doRPC(Client *C)
{
  int rc;
  char c;

  printf("enter (h|m<c>|g): ");
  /*getchar();*/
  scanf("%c", &c);
  rc=doRPCCmd(C,c,0);

  if(proto_debug())fprintf(stderr,"doRPC: rc=0x%x\n", rc);

  return rc;
}


int 
docmd(Client *C, char* cmd)
{
  int rc = 1;
  int move;
  move=atoi(cmd);

  //I AM NOT CHECKING FOR INCORRCT INPUT - KATSU
  //SEGMENTATION FAULT CAN OCCUR 
  //Input must follow this format: connect 255.255.255.255:80000
  if(!connected && strncmp(cmd,"connect",sizeof("connect")-1)==0)
  {
	char address[2][STRLEN]; 
	
	char* token;
	token = strtok(cmd+8,":i\n\0"); //Tokenize C-String for ip
	if (token == NULL) 
	{
		fprintf(stderr, "Please specify as such >connect <host>:<port>"); 
		return -1;
	}
	strcpy(address[0],token); 	//Put ip address into address
	
	token = strtok(NULL,":i\n\0"); //Tokenize C-String for port
	if (token == NULL) 
	{
		fprintf(stderr,"Please specify as such >connect <host>:<port>"); 
		return -1;
	}
	strcpy(address[1],token); 	//put port number into address
	

	initGlobals(2,address);
	// ok startup our connection to the server
	if (startConnection(C, globals.host, globals.port, update_event_handler)<0) 
	{
		fprintf(stderr, "ERROR: Not able to connect to %s:%d\n",globals.host,(int)globals.port);
		rc = -2;
 	 }
  	
	rc=doRPCCmd(C,'h',0);
  }
  else if(strncmp(cmd,"where",5)==0)
  {
  	if (connected) printf("Host = %s : Port = %d", globals.host , (int) globals.port );
	else printf("Not connected\n");
  }
  else if( connected )
  {
	  if( strncmp(cmd,"disconnect",10)==0)
	  {  
		connected = 0;
		set_player(0);	
		rc=doRPCCmd(C,'g',0);
	  }
	  else if(strncmp(cmd,"\n",1)==0)
	  {
		board_print(&gameboard);
	  }
	  else if(move>=0 && move<9) //I don't think this is really not a safe check....
	  { 
	  	rc=doRPCCmd(C,'m',*cmd);
	  }
  }
  else
  {
  	printf("Please Connect First\n");
  }

  return rc;
}

void *
shell(void *arg)
{
  Client *C = arg;
  char *c;
  int rc;
  int menu=1;

  while (1) {
    if ((c = prompt(menu))!=0) rc=docmd(C, c);
    if (rc == -2) break; //only terminate when client issues 'q'
  }

 if(c!=0)//If this variable was allocated in prompt(menu) please free memory
	 free(c);
	 

  fprintf(stderr, "terminating\n");
  fflush(stdout);
  return NULL;
}

void 
usage(char *pgm)
{
  fprintf(stderr, "USAGE: %s <port|<<host port> [shell] [gui]>>\n"
           "  port     : rpc port of a game server if this is only argument\n"
           "             specified then host will default to localhost and\n"
	   "             only the graphical user interface will be started\n"
           "  host port: if both host and port are specifed then the game\n"
	   "examples:\n" 
           " %s 12345 : starts client connecting to localhost:12345\n"
	  " %s localhost 12345 : starts client connecting to locaalhost:12345\n",
	   pgm, pgm, pgm);
 
}

void
initGlobals(int argc, char argv[][STRLEN])
{
  bzero(&globals, sizeof(globals));

  if (argc==1) {
    usage(argv[0]);
    exit(-1);
  }

  if (argc==2) {
    strncpy(globals.host, argv[0], STRLEN);
    globals.port = atoi(argv[1]);
  }

  if (argc>=3) {
    strncpy(globals.host, argv[1], STRLEN);
    globals.port = atoi(argv[2]);
  }
	
   connected = 1;
}

int 
main(int argc, char **argv)
{
  Client c;
	
 // initGlobals(argc, argv); //NOT NEEDED - Katsu

  if (clientInit(&c) < 0) {
    fprintf(stderr, "ERROR: clientInit failed\n");
    return -1;
  }    

  shell(&c);

  return 0;
}
