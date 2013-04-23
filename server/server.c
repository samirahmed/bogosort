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
#include "../lib/types.h"
#include "../lib/protocol.h"
#include "../lib/net.h"
#include "../lib/game_commons.h"
#include "../lib/game_server.h"
#include "../lib/protocol_session.h"
#include "../lib/protocol_server.h"
#include "../lib/protocol_utils.h"

static Maze maze; 		

///////////////////
//  EVENT CODE   //
///////////////////

int doUpdateClients(Maze*m,EventUpdate *update)
{
  Proto_Session *s;
  Proto_Msg_Hdr hdr;
  
  // Copy players/broken walls into update
  hdr.sver.raw = update->timestamp;
  hdr.pstate.v0.raw = update->game_state_update;
  hdr.pstate.v1.raw = update->compress_player_a;
  hdr.pstate.v2.raw = update->compress_player_b;

  // Push objects into update
  server_request_objects(m,&hdr.pstate.v2.raw,&hdr.pstate.v0.raw,&hdr.pstate.v3.raw,&hdr.pstate.v1.raw);

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
    fprintf(stderr,"Client Lost but FD doesn't match existing player\n");
    return -1;
  }

  server_game_drop_player(&maze, team, id);
	fprintf(stdout, "Client Lost fd%d\n",(int)s->fd);
  
  if (proto_debug()) proto_session_dump(s);
	return -1;
}

///////////////////
// HANDLER CODE ///
///////////////////

int hello_handler( Proto_Session *s)
{
  if (proto_debug()) fprintf(stderr,"Hello received\n");

  int rc;
  Player*player;
  Pos pos;  
  rc = server_game_add_player(&maze,s->fd,&player,&pos);
  if(rc<0) reply(s,PROTO_MT_REP_HELLO,rc,(size_t)NULL);
  
  /// EVENT UPDATE GOES HERE

  Proto_Msg_Hdr h;
  bzero(&h, sizeof(Proto_Msg_Hdr));
  h.type = PROTO_MT_REP_HELLO;
  h.pstate.v0.raw = player->id;
  h.pstate.v1.raw = player->team;
  h.gstate.v0.raw = rc;
  put_hdr(s,&h);

  return reply(s,(size_t)NULL,(size_t)NULL,(size_t)NULL);
}

int goodbye_handler( Proto_Session *s)
{
  if(proto_debug()) fprintf(stderr,"Drop Request Received");
  client_lost_handler(s);
  
  /// EVENT UPDATE GOES HERE
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
 
  return reply(s,(size_t)NULL,(size_t)NULL,(size_t)NULL);
}

int action_handler( Proto_Session *s)
{
  if (proto_debug()) fprintf(stderr,"Action Request Received from\n");

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
  if (rc<0) return reply(s,PROTO_MT_REP_ACTION,rc,-1);
  
  rc = server_game_action(&maze, &request);
  if (rc<0) return reply(s,PROTO_MT_REP_ACTION,rc,-1);

  /// EVENT UPDATE GOES HERE
  
  return reply(s,PROTO_MT_REP_ACTION,rc,request.update.timestamp);
}

int init_game(void){
	int rc;
  rc = maze_build_from_file(&maze,"../daGame.map");
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

char MenuString[] =
  "Usage: d/D-debug on/off\n\tu-update clients\n\tq-quit\n\tload <filename> - Loads a Map file\n\tdump - dump contents of map data structure\n";

int docmd(char* cmd)
{
  int rc = 1;

  if((strncmp(cmd,"dump",4))==0) { maze_dump(&maze); return rc; }

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
