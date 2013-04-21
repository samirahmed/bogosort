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
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include "../lib/types.h"
#include "../lib/protocol.h"
#include "../lib/net.h"
#include "../lib/game_commons.h"
#include "../lib/game_server.h"
#include "../lib/protocol_session.h"
#include "../lib/protocol_server.h"
#include "../lib/protocol_utils.h"

static Maze maze; 		

int client_lost_handler(Proto_Session *);
void init_game(void);
int updateClients(void);

int hello_handler( Proto_Session *s)
{
  fprintf(stderr,"hello received");
  return reply(s,PROTO_MT_REP_BASE_HELLO,5);
}

int goodbye_handler( Proto_Session *s)
{
  fprintf(stderr, "goodbye received");
  return reply(s,PROTO_MT_REP_BASE_GOODBYE,5);
}

int dump_handler( Proto_Session *s)
{
  fprintf(stderr, "dump received");
  maze_dump(&maze);
  return reply(s,PROTO_MT_REP_BASE_DUMP,NULL);
}

extern int client_lost_handler( Proto_Session * s)
{
	fprintf(stdout, "Session lost - Dropping Client ...:\n");
  if (proto_debug()) proto_session_dump(s);
	return -1;
}

extern void init_game(void)
{
	
  // Setup handler for Hello event
 	proto_server_set_req_handler( PROTO_MT_REQ_BASE_HELLO , &(hello_handler) );
 	proto_server_set_req_handler( PROTO_MT_REQ_BASE_GOODBYE , &(goodbye_handler) );
 	proto_server_set_req_handler( PROTO_MT_REQ_BASE_DUMP, &(dump_handler) );

	// Should set a session lost handler here
  proto_server_set_session_lost_handler( &(client_lost_handler) );	
	// Init the lock
}

int 
doUpdateClients(void)
{
  Proto_Session *s;
  Proto_Msg_Hdr hdr;

  s = proto_server_event_session();
  hdr.type = PROTO_MT_EVENT_BASE_UPDATE;
  proto_session_hdr_marshall(s, &hdr);
  proto_server_post_event();  
  return 1;
}

//////////////////
//  SHELL CODE  //
//////////////////

char MenuString[] =
  "Usage: d/D-debug on/off\n\tu-update clients\n\tq-quit\n\tload <filename> - Loads a Map file\n\tdump - dump contents of map data structure\n";

int docmd(char* cmd)
{
  int rc = 1;

  if((strncmp(cmd,"load",4))==0)  return maze_build_from_file(&maze,"../daGame.map");
  else if((strncmp(cmd,"dump",4))==0) { maze_dump(&maze); return rc; }

  switch (*cmd) {
  case 'd':
    proto_debug_on();
    break;
  case 'D':
    proto_debug_off();
    break;
  case 'u':
    rc = doUpdateClients();
    break;
  case 'q':
    rc=-1;
    break;
  case '\n':
  case ' ':
    rc=1;
    break;
  default:
    printf("Unkown Command\n");
  }
  return rc;
}

char* prompt(int menu) 
{
  if (menu) printf("%sSERVER_SHELL$ ", MenuString);
  fflush(stdout);
  
  // Pull in input from stdin
  int bytes_read;
  size_t nbytes = 0;
  char *my_string;
  bytes_read = getline (&my_string, &nbytes, stdin);
  
  char* pch;
  pch = strchr(my_string,'\n'); 
  *pch = '\0'; 			//Remove newline character

  if(bytes_read>0)
	return my_string;
  else
	return 0;
}

void * shell(void *arg)
{
  char* c;
  int rc=1;
  int menu=1;

  while (1) {
    if ((c=prompt(menu))!=0) rc=docmd(c);
    if (rc<0) break;
    if (rc==1) menu=1; else menu=0;
  }
  if(c!=0)//If this variable was allocated in prompt(menu) please free memory
	free(c);

  fprintf(stderr, "terminating\n");
  fflush(stdout);
  return NULL;
}

int main(int argc, char **argv)
{ 
  if (proto_server_init()<0) {
    fprintf(stderr, "ERROR: failed to initialize proto_server subsystem\n");
    exit(-1);
  }
  fprintf(stderr, "Initializing Request Handlers\n");
  init_game();

  fprintf(stderr, "RPC Port: %d, Event Port: %d\n", proto_server_rpcport(), 
	  proto_server_eventport());

  if (proto_server_start_rpc_loop()<0) {
    fprintf(stderr, "ERROR: failed to start rpc loop\n");
    exit(-1);
  }
    
  shell(NULL);

  return 0;
}
