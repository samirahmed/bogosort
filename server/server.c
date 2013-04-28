/******************************************************************************
* Copyright (C) 2013 by Samir Ahmed, Katsu Kawakami, Will Seltzer
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
#include <time.h>
#include "../lib/types.h"
#include "../lib/protocol.h"
#include "../lib/net.h"
#include "../lib/game_commons.h"
#include "../lib/game_server.h"
#include "../lib/protocol_session.h"
#include "../lib/protocol_server.h"
#include "../lib/protocol_utils.h"

static Maze maze; 		
static char timestr[9];
static int  teleport;
//////////////////
// HELPER CODE  //
//////////////////

void slog(char*cmd,int*action,int fd,int* team,int* id,int rc)
{
  time_t raw;
  struct tm * tt;
  char rcstr[25];
  
  time(&raw); 
  tt = localtime(&raw);
  sprintf(timestr,"%.2d:%.2d:%.2d",tt->tm_hour,tt->tm_min,tt->tm_sec);
  
  if (rc >= 0)
    sprintf(rcstr,COLOR_OKGREEN "%d" COLOR_END , rc); 
  else
    sprintf(rcstr,COLOR_FAIL "%d" COLOR_END , rc);

  if (team && id && action )
	fprintf(stdout,
    "%s\t"COLOR_YELLOW"%s"COLOR_END
    "\t[%1d]\tfd:%05d\tteam:"
    COLOR_BLUE"%1d"COLOR_END"\tid:"
    COLOR_BLUE"%03d"COLOR_END"\trc:%s\n",
   timestr,cmd,*action,fd,*team,*id,rcstr);
  else if (team && id)
	fprintf(stdout,
    "%s\t"COLOR_YELLOW"%s"COLOR_END
    "\t\tfd:%05d\tteam:"
    COLOR_BLUE"%1d"COLOR_END"\tid:"
    COLOR_BLUE"%03d"COLOR_END"\trc:%s\n",
    timestr,cmd,fd,*team,*id,rcstr);
  else
	fprintf(stdout,
    "%s\t"COLOR_YELLOW"%s"COLOR_END"\t\tfd:%05d\tteam:_\tid:___\trc:%s\n",
    timestr,cmd,fd,rcstr);
    
  fflush(stdout);
}

///////////////////
//  EVENT CODE   //
///////////////////

int doUpdateClients(Update *update)
{
  Proto_Session *s;
  Proto_Msg_Hdr hdr;
  bzero(&hdr, sizeof(Proto_Msg_Hdr));

  // Copy players/broken walls into update
  hdr.sver.raw = update->timestamp;
  hdr.gstate.v0.raw = update->game_state_update;
  hdr.gstate.v1.raw = update->compress_player_a;
  hdr.gstate.v2.raw = update->compress_player_b;

  // Push & compress objects into update
  int ii;
  for(ii=0; ii<NUM_OBJECTS; ii++)
  {
    Object*object = &update->objects[ii];
    if (object->cell) // all valid should have cells set
    {
      int compress;
      compress_object(object, &compress);
      switch(ii)
      {
        case 0: hdr.pstate.v0.raw = compress; break;
        case 1: hdr.pstate.v1.raw = compress; break;
        case 2: hdr.pstate.v2.raw = compress; break;
        case 3: hdr.pstate.v3.raw = compress; break;
        default: break;
      }
    }
  }

  /*server_request_objects(&maze,&hdr.pstate.v2.raw,&hdr.pstate.v0.raw,&hdr.pstate.v3.raw,&hdr.pstate.v1.raw);*/

  s = proto_server_event_session();
  hdr.type = PROTO_MT_EVENT_UPDATE;
  proto_session_hdr_marshall(s, &hdr);
  proto_server_post_event();  
  return 1;
}

///////////////////
//  LOST  CODE   //
///////////////////

int client_lost_handler( Proto_Session * s)
{
  int id,team,rc,fd;
  fd = (int)s->fd;
  rc =server_fd_to_id_and_team(&maze,fd,&team,&id);
  if (rc <0) 
  {
    if (proto_debug() ) fprintf(stdout,"DIS\t\tfd:%05d\n",fd);
    return -1;
  }

	slog("DRP",NULL,fd,&team,&id,0);
  Update update;  
  server_game_drop_player(&maze, team, id, &update);

  if (proto_debug()) proto_session_dump(s);
  
  /// EVENT UPDATE GOES HERE
  doUpdateClients(&update);

  return -1;
}

///////////////////////
// RPC HANDLER CODE  //
///////////////////////

int hello_handler( Proto_Session *s)
{
  int rc;
  Player*player;
  Update update;
  
  rc = server_game_add_player(&maze,s->fd,&player,&update);
  if(rc<0) reply(s,PROTO_MT_REP_HELLO,rc,(size_t)NULL);
  
  Proto_Msg_Hdr h;
  bzero(&h, sizeof(Proto_Msg_Hdr));
  h.type = PROTO_MT_REP_HELLO;
  h.pstate.v0.raw = player->id;
  h.pstate.v1.raw = player->team;
  h.gstate.v0.raw = rc;
  put_hdr(s,&h);

  /// EVENT UPDATE GOES HERE
  doUpdateClients(&update);

	slog("HEL",NULL,s->fd,(int*)&player->team,(int*)&player->id,rc);

  return reply(s,(size_t)NULL,(size_t)NULL,(size_t)NULL);
}

int goodbye_handler( Proto_Session *s)
{
  client_lost_handler(s);
  return reply(s,PROTO_MT_REP_GOODBYE,0,(size_t)NULL);
}

int sync_handler( Proto_Session *s)
{
  
  Proto_Msg_Hdr h;
  bzero(&h, sizeof(Proto_Msg_Hdr));
  int *walls, *rlist, *blist, blen, rlen, wlen, 
      rshovel, bshovel, bflag, rflag, rc,ii;
  rc = 0;

  // Get the walls + rlist
  walls = server_request_walls(&maze, &wlen);
  rlist = server_request_plist(&maze, TEAM_RED, &rlen);
  blist = server_request_plist(&maze, TEAM_BLUE, &blen);
  
  // Put them into the body
  for (ii=0 ; ii<wlen ;ii++) put_int(s,walls[ii]);
  for (ii=0 ; ii<rlen ;ii++) put_int(s,rlist[ii]);
  for (ii=0 ; ii<rlen ;ii++) put_int(s,blist[ii]);

  // Free memory
  free(walls);
  free(rlist);
  free(blist);
  
  /// Now put in the objects
  server_request_objects(&maze,&rshovel,&rflag,&bshovel,&bflag);
  put_int(s,rflag);
  put_int(s,bflag);
  put_int(s,rshovel);
  put_int(s,bshovel);

  h.type = PROTO_MT_REP_SYNC;
  h.gstate.v0.raw = rc;
  h.pstate.v2.raw = wlen;
  h.pstate.v3.raw = rlen+blen;
  put_hdr(s,&h);
	
  slog("SYN",NULL,s->fd,NULL,NULL,0);
  return reply(s,(size_t)NULL,(size_t)NULL,(size_t)NULL);
}

int action_handler( Proto_Session *s)
{
  Proto_Msg_Hdr h;
  int rc,id,team,action,fd;
  Pos current,next;
  GameRequest request;
  rc = 0;
  fd = s->fd;
  
  /// Extract information 
  bzero(&h, sizeof(Proto_Msg_Hdr)); 
  proto_session_hdr_unmarshall(s, &h);
  action  = h.gstate.v1.raw;
  id      = h.pstate.v0.raw;
  team    = h.pstate.v1.raw;
  proto_session_body_unmarshall_bytes(s, 0, sizeof(Pos), (char*)&current);
  proto_session_body_unmarshall_bytes(s, sizeof(Pos), sizeof(Pos), (char*)&next);

  rc = server_request_init(&maze,&request,fd,action,next.x,next.y);
  request.test_mode = teleport; // allow server to enable teleport

  if (rc<0)
  {
    slog("ACT",&action,fd,&team,&id,rc);
    return reply(s,PROTO_MT_REP_ACTION,rc,-1);
  }
  
  rc = server_game_action(&maze, &request);
  if (rc<0)
  {
    slog("ACT",&action,fd,&team,&id,rc);
    return reply(s,PROTO_MT_REP_ACTION,rc,-1);
  }

  /// EVENT UPDATE GOES HERE
  doUpdateClients(&request.update);
  
  slog("ACT",&action,fd,&team,&id,rc);
  return reply(s,PROTO_MT_REP_ACTION,rc,request.update.timestamp);
}

int init_game(void){
	int rc;
  teleport = 0;
  rc = maze_build_from_file(&maze,"./daGame.map");
  if (rc <0) fprintf(stderr, "ERROR: Failed to build map\n");
  
  if (proto_debug()) fprintf(stderr, "Initializing Request Handlers\n");
  
  // Setup handler for Hello event
 	proto_server_set_req_handler( PROTO_MT_REQ_HELLO, &(hello_handler) );
 	proto_server_set_req_handler( PROTO_MT_REQ_GOODBYE, &(goodbye_handler) );
 	proto_server_set_req_handler( PROTO_MT_REQ_ACTION, &(action_handler) );
 	proto_server_set_req_handler( PROTO_MT_REQ_SYNC, &(sync_handler) );

	// Should set a session lost handler here
  proto_server_set_session_lost_handler( &(client_lost_handler) );	
  
  return rc;
}

//////////////////
//  SHELL CODE  //
//////////////////

char MenuString[] = "Usage:\t d/D-debug on/off\n\t \
u-update clients\n\t \
q-quit\n\t \
asciidump - dump an ascii reprentation of whole maze\n\t \
textdump - dump player/object/cell/state into readable text file\n\n";

int docmd(char* cmd)
{
  int rc = 1;

  if ((strncmp(cmd,"quit",sizeof("quit")-1))==0)
  {
    return -1;
  }
  else if ((strncmp(cmd,"teleport",sizeof("quit")-1))==0)
  {
    teleport = !teleport;
    if (teleport) 
      fprintf(stdout,"Teleportation " COLOR_BLUE "ENABLED" COLOR_END "\n");
    else
      fprintf(stdout,"Teleportation " COLOR_RED "DISABLED" COLOR_END "\n");

    fflush(stdout);
    return 0;
  }
  else if((strncmp(cmd,"textdump",sizeof("textdump")-1))==0) 
  { 
    char* token;
    token = strtok(cmd+sizeof("textdump")-1,": \n\0");
    if (token == NULL )
    {
      fprintf(stderr,"Please specify filename $textdump <filename>\n");
      return rc;
    }
    maze_text_dump(&maze,token); 
    return rc; 
  }
  else if((strncmp(cmd,"asciidump",sizeof("asciidump")-1))==0) 
  { 
    char* token;
    token = strtok(cmd+sizeof("asciidump")-1,": \n\0");
    if (token == NULL )
    {
      fprintf(stderr,"Please specify filename $asciidump <filename>\n");
      return rc;
    }
    maze_ascii_dump(&maze,token); 
    return rc; 
  }

  switch (*cmd) {
  case 'd':
    proto_debug_on();
    break;
  case 'D':
    proto_debug_off();
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
  if (menu) fprintf(stdout,"%sSERVER_SHELL$\n", MenuString);
  fflush(stdout);
  
  // Pull in input from stdin
  int bytes_read;
  size_t nbytes = 0;
  char *my_string = (char*) malloc(1);
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

  while (1) 
  {
    if ((c=prompt(menu))!=0) rc=docmd(c);
    if (rc<0) break;
    if (rc==1) menu=1; else menu=0;
    if(c!=0)	free(c);
  }
  
  if(c!=0)	free(c);
  fflush(stdout);
  fprintf(stderr, "terminating\n");
  return NULL;
}

int main(int argc, char **argv)
{ 
  if (proto_server_init()<0)
  {
    fprintf(stderr, "ERROR: failed to initialize proto_server subsystem\n");
    exit(-1);
  }
  
  int rc = init_game();
  if (rc<0) {fprintf(stderr,"ERROR: Killing Server!\n"); exit(-1); }

  fprintf(stderr, "RPC Port: %d, Event Port: %d\n", proto_server_rpcport(), proto_server_eventport());

  if (proto_server_start_rpc_loop()<0) {
    fprintf(stderr, "ERROR: failed to start rpc loop\n");
    exit(-1);
  }
    
  shell(NULL);

  return 0;
}
