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
#include "../lib/protocol.h"
#include "../lib/maze.h"
#include "../lib/protocol_client.h"
#include "../lib/protocol_utils.h"

#define STRLEN 81

//Function Headers
void initGlobals(int argc, char argv[][STRLEN]);//Had to Change function header to make this work

struct Globals {
  char host[STRLEN];
  PortType port;
} globals;

typedef struct ClientState  {
  int data;
  Proto_Client_Handle ph;
} Client;

struct Request{
  Client * client;
  Proto_Msg_Types type;
  int x;
  int y;
  Team_Types turf;
  Cell_Types cell_type;
} request;

static int connected;
static char MenuString[] = "\n?> ";

static int update_handler(Proto_Session *s )
{
    return 0;
}

static int
clientInit(Client *C)
{
  // Zero global scope
  bzero(C, sizeof(Client));

  // Set connected state to zero
  connected = 0; 
  
  // initialize the client protocol subsystem
  if (proto_client_init(&(C->ph))<0) {
    fprintf(stderr, "client: main: ERROR initializing proto system\n");
    return -1;
  }
 
  // Specify the event channel handlers
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
  bytes_read = getline(&my_string, &nbytes, stdin);

  if(bytes_read>0)
    return my_string;
  else
    return 0;

}

int 
doRPCCmd() 
{
  int rc=-1;
 
  // Unpack the request
  Client *C;
  Proto_Msg_Hdr hdr;
  bzero(&hdr,sizeof(Proto_Msg_Hdr));

  C = request.client;

  switch (request.type) {
  case PROTO_MT_REQ_BASE_HELLO:  
    {
     fprintf(stderr,"HELLO COMMAND ISSUED");
     hdr.type = request.type;
     rc = do_void_rpc(C->ph,&hdr);
     /*rc = proto_client_hello(C->ph);*/
     /*if (proto_debug()) fprintf(stderr,"hello: rc=%x\n", rc);*/
     /*if (rc < 0) fprintf(stderr, "Unable to connect");*/
    }
    break;
  case PROTO_MT_REQ_BASE_CINFO:
      fprintf(stderr,"CINFO command issued x = %d, y= %d",request.x,request.y);
      hdr.type= request.type;
      hdr.gstate.v0.raw = request.x;
      hdr.gstate.v1.raw = request.y;
      rc = do_void_rpc(C->ph,&hdr);
    break;
  case PROTO_MT_REQ_BASE_DIM:
     fprintf(stderr,"Dimension COMMAND ISSUED");
     hdr.type= request.type;
     rc = do_void_rpc(C->ph,&hdr);
     if (rc > 0)
     {
        int col,row;
        get_int(C->ph,0,&col);
        get_int(C->ph,sizeof(int),&row);
        fprintf(stderr,"%d columns(x) by %d rows(y)",col,row);
     }
    break;
  case PROTO_MT_REQ_BASE_NUM:
     fprintf(stderr,"Number request for cell_type %d",request.turf);
     Cell cell;
     cell_init(&cell,0,0,request.turf,request.cell_type,0);
     cell_marshall_into_header(&cell,&hdr);
     hdr.type=request.type;
     rc = do_void_rpc(C->ph,&hdr);
     if (rc > 0)
     {
        int num;
        get_int(C->ph,0,&num);
        fprintf(stderr,"%d total\n",num);
     }
    break;
  case PROTO_MT_REQ_BASE_DUMP:
     fprintf(stderr,"Dump server map issued");
     hdr.type = request.type;
     rc = do_void_rpc(C->ph,&hdr);
    break;
  case PROTO_MT_REQ_BASE_GOODBYE:
     fprintf(stderr,"Goodbye COMMAND ISSUED");
     hdr.type = request.type;
     rc = do_void_rpc(C->ph,&hdr);
    /*rc = proto_client_goodbye(C->ph);*/
    /*printf("Game Over - You Quit");*/
    break;
  default:
    printf("%s: unknown command %d\n", __func__, request.type);
  }
  // NULL MT OVERRIDE ;-)
  if(proto_debug()) fprintf(stderr,"%s: rc=0x%x\n", __func__, rc);
  if (rc == 0xdeadbeef) rc=1;
  printf("rc=1\n");
  return rc;
}

void disconnect (Client *C)
{
  connected = 0;
  Proto_Session *event = proto_client_event_session(C->ph);
  Proto_Session *rpc = proto_client_rpc_session(C->ph);
  close(event->fd);
  close(rpc->fd);
}

int doConnect(Client *C, char* cmd)
{
  int rc;

  char address[2][STRLEN]; 

  char* token;
  token = strtok(cmd+8,":i\n\0"); //Tokenize C-String for ip
  if (token == NULL) 
  {
    fprintf(stderr, "Please specify as such >connect <host>:<port>"); 
    return -1;
  }
  strcpy(address[0],token);   //Put ip address into address

  token = strtok(NULL,":i\n\0"); //Tokenize C-String for port
  if (token == NULL) 
  {
    fprintf(stderr,"Please specify as such >connect <host>:<port>"); 
    return -1;
  }
  strcpy(address[1],token);   //put port number into address

  initGlobals(2,address);

  // ok startup our connection to the server
  connected = 1;      //Change connected state to true
  if (startConnection(C, globals.host, globals.port, update_event_handler)<0) 
  {
    fprintf(stderr, "ERROR: Not able to connect to %s:%d\n",globals.host,(int)globals.port);
    connected =0;
    return -2;
  }

  // configure request parameters
  request.type = PROTO_MT_REQ_BASE_HELLO;
  rc = doRPCCmd();

  return rc;
}

int docmd(Client *C, char* cmd)
{
  int rc = 1;                      // Set up return code var
  bzero(&request,sizeof(request)); // Set up request
  request.client = C;

  if(strncmp(cmd,"quit",sizeof("quit")-1)==0) return -2;

  if(!connected && strncmp(cmd,"connect",sizeof("connect")-1)==0)
  {
    rc = doConnect(C, cmd);
  }
  else if(strncmp(cmd,"where",sizeof("where")-1)==0)
  {
    if (connected) printf("Host = %s : Port = %d", globals.host , (int) globals.port );
  else printf("Not connected\n");
  }
  else if( connected )
  {
    if( strncmp(cmd,"disconnect",sizeof("disconnect")-1)==0)
    {  
    
    request.type = PROTO_MT_REQ_BASE_GOODBYE;
    rc=doRPCCmd();
    
    disconnect(C);
    }
    else if( strncmp(cmd,"numhome",sizeof("numhome")-1)==0)
    {
      int team;
      char* token;
      
      token = strtok(cmd+(sizeof("numhome")-1),":i \n\0");
      if (token == NULL )
      {
        fprintf(stderr,"Please specify team number 1 or 2 e.g '>numhome 2'"); 
        return rc=-1;
      }
      
      team = atoi(token);
      if ( team!=1 && team!=2 )
      {
        fprintf(stderr, "Please specify team number 1 or 2 e.g '>numhome 2'");
        return -1;
      }

      request.type = PROTO_MT_REQ_BASE_NUM;
      request.cell_type = CELL_HOME;
      request.turf = (Team_Types)(team-1);
      rc=doRPCCmd();
    }
    else if( strncmp(cmd,"numjail",sizeof("numjail")-1)==0)
    {
      int team;
      char* token;
      
      token = strtok(cmd+(sizeof("numjail")-1),":i \n\0");
      if (token == NULL )
      {
        fprintf(stderr,"Please specify team number 1 or 2 e.g '>numhome 2'"); 
        return rc=-1;
      }
      
      team = atoi(token);
      if ( team!=1 && team!=2 )
      {
        fprintf(stderr, "Please specify team number 1 or 2 e.g '>numhome 2'");
        return -1;
      }

      request.type = PROTO_MT_REQ_BASE_NUM;
      request.cell_type = CELL_JAIL;
      request.turf = (Team_Types)(team-1);
      rc=doRPCCmd(); 
    }
    else if( strncmp(cmd,"numwall",sizeof("numwall")-1)==0)
    {
      request.type = PROTO_MT_REQ_BASE_NUM;
      request.cell_type = CELL_WALL;
      rc=doRPCCmd(); 
    }
    else if( strncmp(cmd,"numfloor",sizeof("numfloor")-1)==0)
    {
      request.type = PROTO_MT_REQ_BASE_NUM;
      request.cell_type = CELL_WALL;
      rc=doRPCCmd();  
    }
    else if( strncmp(cmd,"dim",sizeof("dim")-1)==0)
    {
      request.type = PROTO_MT_REQ_BASE_DIM;
      rc=doRPCCmd(); 
    }
    else if( strncmp(cmd,"cinfo",sizeof("cinfo")-1)==0)
    {
      int x;
      int y;
      char * token;
      
      token = strtok(cmd+(sizeof("cinfo")-1), ":i, \n\0");
      if (token == NULL )
      {
        fprintf(stderr, "Please specify cell x and y e.g '>cinfo 25,125'");
        return -1;
      }
     
      x = atoi(token);
      token = strtok(NULL, ":i, \n\0");
      if (token == NULL)
      {
        fprintf(stderr, "Please specify cell x and y e.g '>cinfo 25,125'");
        return -1;
      }
      y = atoi(token);

      request.type = PROTO_MT_REQ_BASE_CINFO;
      request.x = x;
      request.y = y;
      rc=doRPCCmd();
    }
    else if( strncmp(cmd,"dump",sizeof("dump")-1)==0)
    {
      request.type = PROTO_MT_REQ_BASE_DUMP;
      rc=doRPCCmd();
    }
  }
  else
  {
    printf("Please connect first i.e 'connect <host>:<port>'\n");
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
  
  //If this variable was allocated in prompt(menu) please free memory
  if(c!=0) free(c);
  }
  
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
  
}

int 
main(int argc, char **argv)
{
  Client c;
  
  if (clientInit(&c) < 0) {
    fprintf(stderr, "ERROR: clientInit failed\n");
    return -1;
  }    

  shell(&c);

  return 0;
}
