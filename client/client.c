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
#include <unistd.h>
#include "client.h"
#include "../lib/types.h"
#include "../lib/protocol.h"
#include "../lib/protocol_client.h"
#include "../lib/protocol_utils.h"
#include "../lib/game_client.h"

//Global Variables
Globals globals;         //Host string and port
static char MenuString[] = "\n?> ";
static Client c;

static int update_handler(Proto_Session *s ){
    Proto_Msg_Hdr hdr;
    proto_session_hdr_unmarshall(s,&hdr);
    Maze* maze = &c.maze;
    update_walls(1,&hdr.gstate.v0.raw,maze);
    update_players(1,&hdr.gstate.v1.raw,maze);
    update_players(1,&hdr.gstate.v2.raw,maze);
    update_objects(1,&hdr.pstate.v0.raw,maze);
    update_objects(1,&hdr.pstate.v1.raw,maze);
    update_objects(1,&hdr.pstate.v2.raw,maze);
    update_objects(1,&hdr.pstate.v3.raw,maze);
    if(proto_debug)
    {
        fprintf(stderr,"Client position x:%d y:%d\n",c.my_player->client_position.x,c.my_player->client_position.y);
    }
    return hdr.version;
}
static int update_event_handler(Proto_Session *s){return 0;}

int startConnection(Client *C, char *host, PortType port, Proto_MT_Handler h)
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

char* prompt(int menu) 
{
  if (menu) printf("%s", MenuString);
  fflush(stdout);
  
  // Pull in input from stdin
  int bytes_read;
  size_t nbytes = 0;
  char *my_string = (char*)malloc(1);
  bytes_read = getline(&my_string, &nbytes, stdin);

  if(bytes_read>0)
    return my_string;
  else
    return 0;

}


void disconnect (Client *C)
{
  C->connected = 0;
  Proto_Session *event = proto_client_event_session(C->ph);
  Proto_Session *rpc = proto_client_rpc_session(C->ph);
  close(event->fd);
  close(rpc->fd);
}

int doConnect(Client *C, char* cmd)
{
  int rc;
  Request request;
  request_sync_init(&request,C);
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

  globals_init(2,address);

  // ok startup our connection to the server
  C->connected = 1;      //Change connected state to true
  if (startConnection(C, globals.host, globals.port, update_event_handler)<0) 
  {
    fprintf(stderr, "ERROR: Not able to connect to %s:%d\n",globals.host,(int)globals.port);
    C->connected =0;
    return -2;
  }

  // configure request parameters
  rc = doRPCCmd(&request);

  return rc;
}

int docmd(Client *C, char* cmd)
{
  int rc = 1;                      // Set up return code var

  if(strncmp(cmd,"quit",sizeof("quit")-1)==0) return -2;

  if(!C->connected && strncmp(cmd,"connect",sizeof("connect")-1)==0)
  {
    rc = doConnect(C, cmd);
    return process_RPC_message(C);
  }
  else if(strncmp(cmd,"where",sizeof("where")-1)==0)
  {
    if (C->connected) printf("Host = %s : Port = %d", globals.host , (int) globals.port );
    else printf("Not connected\n");
  }
  /*else if(strncmp(cmd,"load",sizeof("load")-1)==0)*/
    /*{*/
        /*char* pch;*/
        /*pch = strtok(cmd+5," \n\0");*/
        /*client_map_init(C,pch);*/
    /*}*/
  else if(strncmp(cmd,"debug on",sizeof("debug on")-1)==0)
  {
    proto_debug_on();    
  }
  else if(strncmp(cmd,"debug off",sizeof("debug off")-1)==0)
  {
    proto_debug_off();    
  }
  else if( C->connected )
  {
    Request request;

    if( strncmp(cmd,"disconnect",sizeof("disconnect")-1)==0)
    {  
        request_goodbye_init(&request,C);
        rc=doRPCCmd(&request);

        disconnect(C);
    }
    else if(strncmp(cmd,"move",sizeof("move")-1)==0)
    {
        char* pch;
        Pos next;
        pch = strtok(cmd+5," ");
        next.x = atoi(pch);
        pch = strtok(NULL," ");
        next.y = atoi(pch);
        request_action_init(&request,C,ACTION_MOVE,&C->my_player->client_position,&next);
        rc = doRPCCmd(&request);

    }
    else if(strncmp(cmd,"pickup flag",sizeof("pickup flag")-1)==0)
    {
        request_action_init(&request,C,ACTION_PICKUP_FLAG,NULL,NULL);
        rc = doRPCCmd(&request);

    }
    else if(strncmp(cmd,"drop flag",sizeof("drop flag")-1)==0)
    {
        request_action_init(&request,C,ACTION_DROP_FLAG,NULL,NULL);
        rc = doRPCCmd(&request);
    }
    else if(strncmp(cmd,"pickup shovel",sizeof("pickup shovel")-1)==0)
    {
        request_action_init(&request,C,ACTION_PICKUP_SHOVEL,NULL,NULL);
        rc = doRPCCmd(&request);

    }
    else if(strncmp(cmd,"drop shovel",sizeof("drop shovel")-1)==0)
    {
        request_action_init(&request,C,ACTION_DROP_SHOVEL,NULL,NULL);
        rc = doRPCCmd(&request);
    }
    else if(strncmp(cmd,"hello",sizeof("hello")-1)==0)
    {
        request_hello_init(&request,C);
        rc = doRPCCmd(&request);
    }
    return process_RPC_message(C);
  }
  else
  {
    printf("Please connect first i.e 'connect <host>:<port>'\n");
  }

  return rc;
}

void* shell(void *arg)
{
  Client *C = arg;
 client_map_init(C,"daGame.map"); 
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

void usage(char *pgm)
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

void globals_init(int argc, char argv[][STRLEN])
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

int main(int argc, char **argv)
{
  if (client_init(&c,update_handler) < 0) {
    fprintf(stderr, "ERROR: clientInit failed\n");
    return -1;
  }    

  shell(&c);

  return 0;
}
