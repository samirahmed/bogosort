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
#include "../lib/types.h"
#include "../lib/protocol.h"
#include "../lib/net.h"
#include "../lib/protocol_session.h"
#include "../lib/protocol_server.h"
#include "../lib/protocol_utils.h"

static Proto_Msg_Hdr Game;

void init_game(void);
int updateClients(void);

int hello_handler( Proto_Session * s)
{
	// LOCK
	
	// If game is in a completed state, and somebody issues a hello, we reset the game
	int value;
	value = 1;
	Proto_Msg_Hdr h;
	bzero(&h, sizeof(Proto_Msg_Hdr));
	if ( Game.gstate.v0.raw > 2 )
	{
		init_game();
		value = 1;
	}
	if ( Game.gstate.v1.raw == 0 )
	{
		Game.gstate.v1.raw = 1;
		return reply(s,PROTO_MT_REP_BASE_HELLO,1);
	}
	else if ( Game.gstate.v2.raw == 0 )
	{
		Game.gstate.v2.raw = 2;
		updateClients();
		return reply(s,PROTO_MT_REP_BASE_HELLO,2);
	}
	// UNLOCK
	
	// no player slot are open, respond with a zero.
	return reply(s,PROTO_MT_REP_BASE_HELLO,value,0);
}

int winner(int position)
{
	if ((position & (1<<0)+(1<<1)+(1<<2) ) == (1<<0)+(1<<1)+(1<<2)  ) return 1;
	if ((position & (1<<3)+(1<<4)+(1<<5) ) == (1<<3)+(1<<4)+(1<<5)  ) return 1;
	if ((position & (1<<6)+(1<<7)+(1<<8) ) == (1<<6)+(1<<7)+(1<<8)  ) return 1;
	if ((position & (1<<0)+(1<<3)+(1<<6) ) == (1<<0)+(1<<3)+(1<<6)  ) return 1;
	if ((position & (1<<1)+(1<<4)+(1<<7) ) == (1<<1)+(1<<4)+(1<<7)  ) return 1;
	if ((position & (1<<2)+(1<<5)+(1<<8) ) == (1<<2)+(1<<5)+(1<<8)  ) return 1;
	if ((position & (1<<0)+(1<<4)+(1<<8) ) == (1<<0)+(1<<4)+(1<<8)  ) return 1;
	if ((position & (1<<2)+(1<<4)+(1<<6) ) == (1<<2)+(1<<4)+(1<<6)  ) return 1;
	return 0;
}

// Make sure this is locked when called
int updateClients( )
{ 
	if ( Game.gstate.v1.raw == -1 || Game.gstate.v2.raw == -1 )
	{
		Game.gstate.v0.raw = 6; // Game Ended Abruptly
	}
	else if ( Game.gstate.v1.raw == 0 || Game.gstate.v2.raw == 0 )
	{
		Game.gstate.v0.raw = 0; // Waiting for one more partner
	}
	else
	{
		int p1;
		int p2;
		p1 = Game.pstate.v0.raw ;
		p2 = Game.pstate.v1.raw ;
		int state = Game.gstate.v0.raw;
		if ( winner(p1) )
		{
			Game.gstate.v0.raw = 3; // Player 1 wins
		}
		else if( winner(p2) )
		{
			Game.gstate.v0.raw = 4; // player 2 Wins
		}
		else if (  ( p1 | p2 ) == (1<<9)-1 )
		{
			Game.gstate.v0.raw = 5; // Stalemate
		}
		else if ( state == 1 )
		{
			Game.gstate.v0.raw = 2; // Now its player twos turn
		}
		else if ( state == 2 )
		{
			Game.gstate.v0.raw = 1; // Now its player ones turn
		}
		else if ( state == 0 )
		{
			Game.gstate.v0.raw = 1; // Start the game, its player 1's turn
		}
	}
	Proto_Session *s;
	s = proto_server_event_session();
	proto_session_hdr_marshall(s, &Game );
	proto_server_post_event();  
	return 1;
}


int goodbye_handler( Proto_Session *s)
{
	// LOCK
	
	Proto_Msg_Hdr h;
	bzero(&h, sizeof(Proto_Msg_Hdr));
	proto_session_hdr_unmarshall(s, &h);
	
	int client_id = h.pstate.v2.raw;
	if ( client_id == 1) Game.gstate.v1.raw = -1;
	else  Game.gstate.v2.raw = -1;
	
	updateClients();

	// UNLOCK
	return 0;
}

int move_handler( Proto_Session *s )
{
	int desired;
	int p1;
	int p2;
	int current_state;
	int client_id;
	Proto_Msg_Hdr h;
	
	// LOCK
	//
	// Unmarshall Header and Body
	bzero(&h, sizeof(Proto_Msg_Hdr));
	proto_session_hdr_unmarshall(s, &h);
	proto_session_body_unmarshall_int(s,0,&desired);

	// Get the current state
	current_state = Game.gstate.v0.raw;
	client_id = h.pstate.v2.raw;

	// If the game hasn't started this is an invalid move
	if ( current_state == 0 ) return reply(s, PROTO_MT_REP_BASE_MOVE, -1 ); 

	// if GAME MODE == CLIENT_ID (1's Move or 2's Move) 
	// obviously this can be spoofed by another client and 
	if ( current_state != client_id ) return  reply(s, PROTO_MT_REP_BASE_MOVE, 0 );
	// It is not this clients turn to move

	// Invalid if the desired move is between 0-8 (inclusive)
	if ( desired > 8 || desired < 0) return  reply(s, PROTO_MT_REP_BASE_MOVE, -1 );
	
	// Create the new player states
	p1 = Game.pstate.v0.raw;
	p2 = Game.pstate.v1.raw;
	
	// Update the player states to add the desired move
	if ( client_id == 1 )
	{
		p1 = p1 | (1 << desired);
		if (p1 == Game.pstate.v0.raw ) return  reply(s, PROTO_MT_REP_BASE_MOVE, -1 ); // If the move makes no difference
	}
	else 
	{
		p2 = p2 | (1 << desired);
		if (p2 == Game.pstate.v1.raw) return reply(s, PROTO_MT_REP_BASE_MOVE, -1 ); // If the move makes no difference
	}
	
	// Now check if the move doesnt violate the other players previous moves
	if (p1 & p2 ) return reply(s, PROTO_MT_REP_BASE_MOVE, -1 ); 

	// If this is valid. Update the Game
	Game.pstate.v0.raw = p1;
	Game.pstate.v1.raw = p2;

	updateClients();
	
	// UNLOCK
	
	return reply(s, PROTO_MT_REP_BASE_MOVE, 1);
}

extern void init_game(void)
{
	
	// Create new game and set player ids to -1
	bzero(&Game, sizeof(Proto_Msg_Hdr));
	Game.gstate.v1.raw = 0;
	Game.gstate.v2.raw = 0;
	Game.type = PROTO_MT_EVENT_BASE_UPDATE;

	// Setup handler for Hello event
 	proto_server_set_req_handler( PROTO_MT_REQ_BASE_HELLO , &(hello_handler) );
 	proto_server_set_req_handler( PROTO_MT_REQ_BASE_MOVE , &(move_handler) );
 	proto_server_set_req_handler( PROTO_MT_REQ_BASE_GOODBYE , &(goodbye_handler) );

	// Should set a session lost handler here

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
docmd(char cmd)
{
  int rc = 1;

  switch (cmd) {
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

int
prompt(int menu) 
{
  int ret;
  int c=0;

  if (menu) printf("%s:", MenuString);
  fflush(stdout);
  c=getchar();;
  return c;
}

void *
shell(void *arg)
{
  int c;
  int rc=1;
  int menu=1;

  while (1) {
    if ((c=prompt(menu))!=0) rc=docmd(c);
    if (rc<0) break;
    if (rc==1) menu=1; else menu=0;
  }
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
