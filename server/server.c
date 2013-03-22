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

static pthread_mutex_t maplock;
static Maze map; 		//Static Global variable for the map
int client_lost_handler(Proto_Session *);
void init_game(void);
int updateClients(void);

void fillMaze(char buffer[][MAX_COL_MAZE],int max_x, int max_y){
//	|---------->X Axis
//	|
//	|
//	|
// 	V
// 	 Y Axis
	int x,y;	
	for(y=0;y<max_y;y++) 				//Y Axis = Row
		for(x=0;x<max_x;x++){ 			//X Axis = Column
			cell_init(&(map.pos[x][y]), 		//pos[x][y]
				  x,
				  y,
				  getTurfType(x),
				  getCellType(buffer[y][x],max_x),  	//buffer[row][column] = buffer[y][x]
				  getMutableType(buffer[y][x],x,y,max_x,max_y));
		}
}


int loadMaze(char* filename){
	FILE* fp;
	char buffer[MAX_ROW_MAZE][MAX_COL_MAZE];		//This size buffer is okay for now
	int rowLen = 0; 			//Number of chars in one line of the file
	int colLen = 0; 			//Index for row into the buffer
	fp = fopen(filename,"r"); 		//Open File
	if(fp==NULL){ 				//Check if file was correctly open
		fprintf(stderr,"Error opening file %s for reading\n",filename);
		return -1;
	}
	else{ 					//Read in the file
		fgets(buffer[rowLen],MAX_COL_MAZE,fp);
		colLen = strlen(buffer[rowLen++]);
		while((fgets(buffer[rowLen++],MAX_COL_MAZE,fp))!=NULL);
		colLen--;
		rowLen--;		
		maze_init(&map,colLen,rowLen);
		fillMaze(buffer,colLen,rowLen);
	}
	if((fclose(fp))!=0) 			//Close the file and check for error
		fprintf(stderr,"Error closing file");
	return 1;
}

void dumpMaze(){
	int x,y;
	FILE* dumpfp;
	dumpfp = fopen("dumpfile.text","w");
	for(y=0;y<map.max_y;y++){
		for(x=0;x<map.max_x;x++){
			if(map.pos[x][y].type==CELL_WALL)
			{
				fprintf(stdout,"#");
				fprintf(dumpfp,"#");

			}
			else if(map.pos[x][y].type==CELL_FLOOR)
			{
				fprintf(stdout," ");
				fprintf(dumpfp," ");
			}
			else if(map.pos[x][y].type==CELL_JAIL && map.pos[x][y].turf==TEAM_RED)
			{
				fprintf(stdout,"j");
				fprintf(dumpfp,"j");
			}
			else if(map.pos[x][y].type==CELL_HOME && map.pos[x][y].turf==TEAM_RED)
			{
				fprintf(stdout,"h");
				fprintf(dumpfp,"h");
			}
			else if(map.pos[x][y].type==CELL_JAIL && map.pos[x][y].turf==TEAM_BLUE)
			{
				fprintf(stdout,"J");
				fprintf(dumpfp,"J");
			}
			else if(map.pos[x][y].type==CELL_HOME && map.pos[x][y].turf==TEAM_BLUE)
			{
				fprintf(stdout,"H");
				fprintf(dumpfp,"H");
			}
		}
		fprintf(stdout,"\n");
		fprintf(dumpfp,"\n");
	}
	close(dumpfp);

}

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

int num_handler( Proto_Session *s)
{
  Cell cell;
  Proto_Msg_Hdr hdr;
  bzero(&hdr, sizeof(Proto_Msg_Hdr)); 
  bzero(&cell, sizeof(Cell)); 
  proto_session_hdr_unmarshall(s, &hdr); 
  cell_unmarshall_from_header(&cell,&hdr);

  int count;
  count = 0;

  int col;
  int row;
  for (col = 0; col < map.max_x; col++ ) 
  {
    for( row = 0; row < map.max_y; row++)
    {
      if (map.pos[col][row].type == cell.type)
      {
        switch( cell.type )
        {
            case CELL_WALL:
              count++;
              break;
            case CELL_FLOOR:
              count++;
              break;
            default:
              if (map.pos[col][row].turf == cell.turf) count++;
            break;
        }
      }
    }
  }
  
  return reply(s,PROTO_MT_REP_BASE_NUM,count);
}

int dim_handler( Proto_Session *s)
{
  fprintf(stderr, "dim received");
  
  put_int(s,map.max_x);
  put_int(s,map.max_y);
  reply(s,PROTO_MT_REP_BASE_DIM,NULL);
}

int cinfo_handler( Proto_Session *s)
{
  
  Proto_Msg_Hdr h;
  bzero(&h, sizeof(Proto_Msg_Hdr)); 
  proto_session_hdr_unmarshall(s, &h);
  
  int x;
  int y;
  int is_valid;
  x= h.gstate.v0.raw;
  y= h.gstate.v1.raw;
  fprintf(stderr, "cinfo received for x= %d, y=%d \n",x,y);

  is_valid = (x >= map.min_x && 
             x<map.max_x && 
             y >= map.min_y && 
             y <map.max_y ) ? 1 : 0; 

  if (is_valid)
  {
     Proto_Msg_Hdr shdr;
     bzero(&shdr, sizeof(Proto_Msg_Hdr)); 
     Cell cell ;
     cell = map.pos[x][y];
     cell_marshall_into_header(&cell,&shdr);
     put_hdr(s,&shdr);
  }

  return reply(s,NULL,is_valid);
}

int dump_handler( Proto_Session *s)
{
  fprintf(stderr, "dump received");
  dumpMaze(); 
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
  "Usage: d/D-debug on/off\n\tu-update clients\n\tq-quit\n\tload <filename> - Loads a Map file\n\tdump - dump contents of map data structure\n";

int 
docmd(char* cmd)
{
  int rc = 1;

  if((strncmp(cmd,"load",4))==0)	//Lazily put this here
	return  loadMaze(cmd+5);
  else if((strncmp(cmd,"dump",4))==0){	//Lazily put this here
	dumpMaze();
	return rc;
  }

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
  if (menu) printf("%sSERVER_SHELL$ ", MenuString);
  fflush(stdout);
  
  // Pull in input from stdin
  int bytes_read;
  int nbytes = 0;
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
