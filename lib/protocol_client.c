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
#include <strings.h>
#include <errno.h>
#include <pthread.h>

#include "protocol.h"
#include "protocol_session.h"
#include "protocol_utils.h"
#include "protocol_client.h"
#include "assert.h"
#define NOT_IMPL assert(0)


typedef struct {
  Proto_Session rpc_session;
  Proto_Session event_session;
  pthread_t EventHandlerTid;
  Proto_MT_Handler session_lost_handler;
  Proto_MT_Handler base_event_handlers[PROTO_MT_EVENT_BASE_RESERVED_LAST 
				       - PROTO_MT_EVENT_BASE_RESERVED_FIRST
				       - 1];
} Proto_Client;

extern Proto_Session *
proto_client_rpc_session(Proto_Client_Handle ch)
{
  Proto_Client *c = ch;
  return &(c->rpc_session);
}

extern Proto_Session *
proto_client_event_session(Proto_Client_Handle ch)
{
  Proto_Client *c = ch;
  return &(c->event_session);
}

extern int
proto_client_set_session_lost_handler(Proto_Client_Handle ch, Proto_MT_Handler h)
{
  Proto_Client *c = ch;
  c->session_lost_handler = h;
  return 1;
}

extern int
proto_client_set_event_handler(Proto_Client_Handle ch, Proto_Msg_Types mt,
			       Proto_MT_Handler h)
{
  int i;
  Proto_Client *c = ch;

  if (mt>PROTO_MT_EVENT_BASE_RESERVED_FIRST && 
      mt<PROTO_MT_EVENT_BASE_RESERVED_LAST) {
    i=mt - PROTO_MT_EVENT_BASE_RESERVED_FIRST - 1;
    c->base_event_handlers[i] = h;
    return 1;
  } else {
    return -1;
  }
}

static int 
proto_client_session_lost_default_hdlr(Proto_Session *s)
{
  fprintf(stderr, "Session lost...:\n");
  proto_session_dump(s);
  return -1;
}

static int 
proto_client_event_null_handler(Proto_Session *s)
{
  fprintf(stderr, 
	  "proto_client_event_null_handler: invoked for session:\n");
  proto_session_dump(s);

  return 1;
}

static void *
proto_client_event_dispatcher(void * arg)
{
  Proto_Client *c;
  Proto_Session *s;
  Proto_Msg_Types mt;
  Proto_MT_Handler hdlr;

  pthread_detach(pthread_self());

  /*NOT_IMPL;i*/

  c = arg; 
  s = &(c->event_session);

  for (;;) {
    if (proto_session_rcv_msg(s)==1) {
      mt = proto_session_hdr_unmarshall_type(s);
      if (mt > PROTO_MT_EVENT_BASE_RESERVED_FIRST && 
	  mt < PROTO_MT_EVENT_BASE_RESERVED_LAST) {
		hdlr = c->base_event_handlers[mt-PROTO_MT_EVENT_BASE_RESERVED_FIRST-1];
	/*NOT_IMPL;//ADD CODE*/
	if (hdlr(s)<0) goto leave;
      }
    } else {
      c->session_lost_handler(s);
	  /*NOT_IMPL;//ADD CODE*/
      goto leave;
    }
  }
 leave:
  close(s->fd);
  return NULL;
}

extern int
proto_client_init(Proto_Client_Handle *ch)
{
  Proto_Msg_Types mt;
  Proto_Client *c;
 
  c = (Proto_Client *)malloc(sizeof(Proto_Client));
  if (c==NULL) return -1;
  bzero(c, sizeof(Proto_Client));

  proto_client_set_session_lost_handler(c, 
			      	proto_client_session_lost_default_hdlr);

  for (mt=PROTO_MT_EVENT_BASE_RESERVED_FIRST+1;
       mt<PROTO_MT_EVENT_BASE_RESERVED_LAST; mt++)
   	proto_client_set_event_handler(c, mt, &proto_client_event_null_handler );
   /*NOT_IMPL;*/
	//ADD CODE

  *ch = c;
  return 1;
}

int
proto_client_connect(Proto_Client_Handle ch, char *host, PortType port)
{
  Proto_Client *c = (Proto_Client *)ch;

  if (net_setup_connection(&(c->rpc_session.fd), host, port)<0) 
    return -1;

  if (net_setup_connection(&(c->event_session.fd), host, port+1)<0) 
    return -2; 

  if (pthread_create(&(c->EventHandlerTid), NULL, 
		     &proto_client_event_dispatcher, c) !=0) {
    fprintf(stderr, 
	    "proto_client_init: create EventHandler thread failed\n");
    perror("proto_client_init:");
    return -3;
  }

  return 0;
}

extern int get_int(Proto_Client_Handle ch, int offset, int* result)
{
  int rc;
  Proto_Session *s;
  Proto_Client *c = ch;
  s = &(c->rpc_session);
  
  rc = proto_session_body_unmarshall_int(s, offset, result);
  return rc;
}

extern void get_hdr(Proto_Client_Handle ch, Proto_Msg_Hdr * hdr)
{
  Proto_Session *s;
  Proto_Client *c = ch;
  s = &(c->rpc_session);
  proto_session_hdr_unmarshall(s,hdr);
}

extern void get_pos(Proto_Client_Handle ch, Pos* current)
{
  Proto_Session *s;
  Proto_Client *c = ch;
  s = &(c->rpc_session);
  proto_session_body_unmarshall_bytes(s,0,sizeof(Pos),(char*)current);
}

extern int get_compress_from_body(Proto_Client_Handle ch,int offset ,int num_elements,int* alloc_pointer)
{
  int ii, current_offset;
  int* ptr_to_array;
  current_offset = offset;
  ptr_to_array = alloc_pointer;
  
  Proto_Session *s;
  Proto_Client *c = ch;
  s = &(c->rpc_session);
  
  for(ii = 0; ii<num_elements;ii++)
  {
      current_offset = proto_session_body_unmarshall_int(s,current_offset,ptr_to_array);
      ptr_to_array++;
  }
  return current_offset;
}

//This function is used for RPC's that do not contain anything in the body
//Parameter: Proto_Msg_Hdr ch - that contains all necessary information for the RPC
//           Proto_Client_Handle *h - Handle to the Proto_Client 
//Returns:   Return Code - as specified in Game_Error_Types in types.h
extern int 
do_no_body_rpc(Proto_Client_Handle ch, Proto_Msg_Hdr * h)
{
  int rc;
  Proto_Session *s;
  Proto_Client *c = ch;
  
  s = &(c->rpc_session);
  proto_session_hdr_marshall(s,h);
  
  rc = proto_session_send_msg(s,1);
  rc = proto_session_rcv_msg(s);
  
  if(rc<=0) rc =-1;

  return rc;
}

//This function is used for RPC's that do not contain anything in the body
//Parameter: Proto_Msg_Hdr *h - that contains all necessary information for the RPC
//           Proto_Client_Handle ch - Handle to the Proto_Client 
//           Pos current - current position of the player
//           Pos next - next position that the players wants to move to
//Returns:   Return Code - as specified in Game_Error_Types in types.h
extern int 
do_action_request_rpc(Proto_Client_Handle ch, Proto_Msg_Hdr * hdr,Pos current, Pos next)
{
  int rc;
  Proto_Session *s;
  Proto_Client *c = ch;
  
  s = &(c->rpc_session);
  proto_session_hdr_marshall(s,hdr);
  if( hdr->gstate.v1.raw >= ACTION_MOVE && 
      hdr->gstate.v1.raw <= ACTION_PICKUP_SHOVEL)
  {
      proto_session_body_marshall_bytes(s,sizeof(Pos),(char*)&current);
      proto_session_body_marshall_bytes(s,sizeof(Pos),(char*)&next);
  }
  
  rc = proto_session_send_msg(s,1);
  rc = proto_session_rcv_msg(s);
  
  if(rc<=0) rc =-1;
  return rc;
}



