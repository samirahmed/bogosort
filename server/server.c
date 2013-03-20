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
#include <sys/types.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include "../lib/types.h"
#include "../lib/protocol.h"
#include "../lib/net.h"
#include "../lib/maze.h"
#include "../lib/protocol_session.h"
#include "../lib/protocol_server.h"
#include "../lib/protocol_utils.h"
#include "../lib/maze.h"

static pthread_mutex_t maplock;
static Maze* map; 		//Static Global variable for the map
int client_lost_handler(Proto_Session *);
void init_game(void);
int updateClients(void);

void fillMaze(char** buffer,int max_x, int max_y){
	int ii,jj;	
	for(ii=0;ii<max_x;ii++)
		for(jj=0;jj<max_y;jj++){
			cell_init(&(map->pos[ii][jj]),
				  ii,
				  jj,
				  getTurfType(jj),
				  getCellType(buffer[ii][jj]),
				  getMutableType(buffer[ii][jj],ii,jj));
		}
}


int loadMaze(char* filename){
	FILE* fp;
	char buffer[1000][1000];		//This size buffer is okay for now
	int rowLen = 0; 			//Number of chars in one line of the file
	int colLen = 0; 			//Index for row into the buffer
	fp = fopen(filename,"r"); 		//Open File
	if(fp=NULL){ 				//Check if file was correctly open
		fprintf(stderr,"Error opening file %s for reading\n",filename);
		return -1;
	}
	else{ 					//Read in the file
		fgets(buffer[rowLen],1000,fp);
		colLen = strlen(buffer[rowLen++]);
		while((fgets(buffer[rowLen++],1000,fp))!=NULL);
		maze_init(map,rowLen,colLen);
		fillMaze(buffer,rowLen,colLen);
	}
	if((fclose(fp))!=0) 			//Close the file and check for error
		fprintf(stderr,"Error closing file");
	return 1;
}

int dumpMaze(){


}

int hello_handler( Proto_Session *s)
{
  fprintf(stderr,"hello received");
  return reply(s,PROTO_MT_REP_BASE_HELLO,5);
}

int goodbye_handler( Proto_Session *s)
{
  fprintf(stderr, "hello received");
  return reply(s,PROTO_MT_REP_BASE_GOODBYE,5);
}

int num_handler( Proto_Session *s)
{
  return reply(s,PROTO_MT_REP_BASE_NUM,5);
}

int dim_handler( Proto_Session *s)
{
  fprintf(stderr, "dim received");
  
  put_int(s,127);
  put_int(s,182);
  reply(s,PROTO_MT_REP_BASE_DIM,NULL);
}

int cinfo_handler( Proto_Session *s)
{
  fprintf(stderr, "cinfo received");
  return reply(s,PROTO_MT_REP_BASE_CINFO,5);
}

int dump_handler( Proto_Session *s)
{
  fprintf(stderr, "dump received");
  return reply(s,PROTO_MT_REP_BASE_DUMP,5);
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
 	proto_server_set_req_handler( PROTO_MT_REQ_BASE_DIM, &(dim_handler) );
 	proto_server_set_req_handler( PROTO_MT_REQ_BASE_NUM, &(num_handler) );
 	proto_server_set_req_handler( PROTO_MT_REQ_BASE_CINFO, &(cinfo_handler) );

	// Should set a session lost handler here
  proto_server_set_session_lost_handler( &(client_lost_handler) );	
	// Init the lock
	pthread_mutex_init(&maplock,0);
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

char MenuString[] =
  "d/D-debug on/off u-update clients q-quit";

int 
docmd(char* cmd)
{
  int rc = 1;

  if((strncmp(cmd,"load",4))==0)	//Lazily put this here
	loadMaze(cmd+5);

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

void *
shell(void *arg)
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

int
main(int argc, char **argv)
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
