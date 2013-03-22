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
#include <arpa/inet.h>
#include <sys/types.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>

#include "net.h"
#include "protocol.h"
#include "protocol_utils.h"
#include "protocol_session.h"


#include "assert.h"
#define NOT_IMPL assert(0)

// Debugging Helper that prints all the fields in the Protocol Session Object
// Dumps the Send and Receive Headers/Buffers and length
extern void
proto_session_dump(Proto_Session *s)
{
  fprintf(stderr, "Session s=%p:\n", s);
  fprintf(stderr, " fd=%d, extra=%p slen=%d, rlen=%d\n shdr:\n  ", 
	  s->fd, s->extra,
	  s->slen, s->rlen);
  proto_dump_msghdr(&(s->shdr));
  fprintf(stderr, " rhdr:\n  ");
  proto_dump_msghdr(&(s->rhdr));
}

// Clears the Session Struct zeroing everything
// This includes the headers and buffers
extern void
proto_session_init(Proto_Session *s)
{
  if (s) bzero(s, sizeof(Proto_Session));
}

// Zeros The Send Header and resets the buffer length
// essentially clearing the send Buffer
extern void
proto_session_reset_send(Proto_Session *s)
{
  bzero(&s->shdr, sizeof(s->shdr));
  s->slen = 0;
}

// Reset the recieve Header and the receive buffer
// by setting the rlen to zero
extern void
proto_session_reset_receive(Proto_Session *s)
{
  bzero(&s->rhdr, sizeof(s->rhdr));
  s->rlen = 0;
}

/*******************************************/
/* HEADER MARSHALL/UNMARSHALLING FUNCTIONS */
/*******************************************/

// HOST->NETWORK
// Copies the State Version from the Proto_Session in the object v
// Uses long long because the of the size of the state version
static void
proto_session_hdr_marshall_sver(Proto_Session *s, Proto_StateVersion v)
{
  s->shdr.sver.raw = htonll(v.raw);
}

// NETWORK->HOST
// Copies the State version from the recieved Header in to the object v
// note again the use of the ntohll
static void
proto_session_hdr_unmarshall_sver(Proto_Session *s, Proto_StateVersion *v)
{
  v->raw = ntohll(s->rhdr.sver.raw);
}

// HOST->NETWORK
// Copies and converts each int from the player state object into the
// Proto_Session's send header pstate
static void
proto_session_hdr_marshall_pstate(Proto_Session *s, Proto_Player_State *ps)
{
    s->shdr.pstate.v0.raw  = htonl(ps->v0.raw);
    s->shdr.pstate.v1.raw  = htonl(ps->v1.raw);
    s->shdr.pstate.v2.raw  = htonl(ps->v2.raw);
    s->shdr.pstate.v3.raw  = htonl(ps->v3.raw);
}

// NETWORK->HOST
// Copies and converts the Recieved Headers player state
// into the ps Player State object 
static void
proto_session_hdr_unmarshall_pstate(Proto_Session *s, Proto_Player_State *ps)
{
    ps->v0.raw = ntohl(s->rhdr.pstate.v0.raw);
    ps->v1.raw = ntohl(s->rhdr.pstate.v1.raw);
    ps->v2.raw = ntohl(s->rhdr.pstate.v2.raw);
    ps->v3.raw = ntohl(s->rhdr.pstate.v3.raw);
}

// HOST->NETWORK
// Sets the Send Header Gstate from the gs object
static void
proto_session_hdr_marshall_gstate(Proto_Session *s, Proto_Game_State *gs)
{
    s->shdr.gstate.v0.raw  = htonl(gs->v0.raw);
    s->shdr.gstate.v1.raw  = htonl(gs->v1.raw);
    s->shdr.gstate.v2.raw  = htonl(gs->v2.raw);
}

// NETWORK->HOST
// Copies and converts the Received Header into the gs object
static void
proto_session_hdr_unmarshall_gstate(Proto_Session *s, Proto_Game_State *gs)
{
    gs->v0.raw = ntohl(s->rhdr.gstate.v0.raw);
    gs->v1.raw = ntohl(s->rhdr.gstate.v1.raw);
    gs->v2.raw = ntohl(s->rhdr.gstate.v2.raw);
}

// NETWORK->HOST
// Return current length of the Reciever Buffer
static int
proto_session_hdr_unmarshall_blen(Proto_Session *s)
{
  return ntohl(s->rhdr.blen);
}

// HOST->NETWORK
// Copy the MT into the Proto_session Send Header 
static void
proto_session_hdr_marshall_type(Proto_Session *s, Proto_Msg_Types t)
{
  s->shdr.type = htonl(t);   
}

// NETWORK->HOST
// Deserialize and return the Received Message's Type
extern Proto_Msg_Types
proto_session_hdr_unmarshall_type(Proto_Session *s)
{
    return ntohl(s->rhdr.type);
}

// NETWORK->HOST
// Return the Protocol version of the recieved message
static int
proto_session_hdr_unmarshall_version(Proto_Session *s)
{
    return ntohl(s->rhdr.version);
}

// Given an (presumably empty) Header object
// Copy all the fields from the Proto_Session response message into this header
// This header is now a Host formatted Header
extern void
proto_session_hdr_unmarshall(Proto_Session *s, Proto_Msg_Hdr *h)
{
  
  h->version = proto_session_hdr_unmarshall_version(s);
  h->type = proto_session_hdr_unmarshall_type(s);
  proto_session_hdr_unmarshall_sver(s, &h->sver);
  proto_session_hdr_unmarshall_pstate(s, &h->pstate);
  proto_session_hdr_unmarshall_gstate(s, &h->gstate);
  h->blen = proto_session_hdr_unmarshall_blen(s);
}
   
// Given an Header struct that has been configured by the user. Copy all the
// fields into the Proto Sessions Send Header struct.
extern void
proto_session_hdr_marshall(Proto_Session *s, Proto_Msg_Hdr *h)
{
  // ignore the version number and hard code to the version we support
  s->shdr.version = PROTOCOL_BASE_VERSION;
  proto_session_hdr_marshall_type(s, h->type);
  proto_session_hdr_marshall_sver(s, h->sver);
  proto_session_hdr_marshall_pstate(s, &h->pstate);
  proto_session_hdr_marshall_gstate(s, &h->gstate);
  // we ignore the body length as we will explicity set it
  // on the send path to the amount of body data that was
  // marshalled.
}

/****************************************/
/* Body Marshalling/ UnMarshalling Code */
/****************************************/

// All the following code does is serialize into the buffer
// Or deserialize from the buffer
// Note that serializing into the body will increment the buffer size slen
// Also note that bounds are checkout before memory access

extern int 
proto_session_body_marshall_ll(Proto_Session *s, long long v)
{
  if (s && ((s->slen + sizeof(long long)) < PROTO_SESSION_BUF_SIZE)) {
    *((int *)(s->sbuf + s->slen)) = htonll(v);
    s->slen+=sizeof(long long);
    return 1;
  }
  return -1;
}

extern int 
proto_session_body_unmarshall_ll(Proto_Session *s, int offset, long long *v)
{
  if (s && ((s->rlen - (offset + sizeof(long long))) >=0 )) {
    *v = *((long long *)(s->rbuf + offset));
    *v = ntohll(*v);
    return offset + sizeof(long long);
  }
  return -1;
}

extern int 
proto_session_body_marshall_int(Proto_Session *s, int v)
{
  if (s && ((s->slen + sizeof(int)) < PROTO_SESSION_BUF_SIZE)) {
    *((int *)(s->sbuf + s->slen)) = htonl(v);
    s->slen+=sizeof(int);
    return 1;
  }
  return -1;
}

extern int 
proto_session_body_unmarshall_int(Proto_Session *s, int offset, int *v)
{
  if (s && ((s->rlen  - (offset + sizeof(int))) >=0 )) {
    *v = *((int *)(s->rbuf + offset));
    *v = ntohl(*v);
    return offset + sizeof(int);
  }
  return -1;
}

extern int 
proto_session_body_marshall_char(Proto_Session *s, char v)
{
  if (s && ((s->slen + sizeof(char)) < PROTO_SESSION_BUF_SIZE)) {
    s->sbuf[s->slen] = v;
    s->slen+=sizeof(char);
    return 1;
  }
  return -1;
}

extern int 
proto_session_body_unmarshall_char(Proto_Session *s, int offset, char *v)
{
  if (s && ((s->rlen - (offset + sizeof(char))) >= 0)) {
    *v = s->rbuf[offset];
    return offset + sizeof(char);
  }
  return -1;
}

extern int
proto_session_body_reserve_space(Proto_Session *s, int num, char **space)
{
  if (s && ((s->slen + num) < PROTO_SESSION_BUF_SIZE)) {
    *space = &(s->sbuf[s->slen]);
    s->slen += num;
    return 1;
  }
  *space = NULL;
  return -1;
}

extern int
proto_session_body_ptr(Proto_Session *s, int offset, char **ptr)
{
  if (s && ((s->rlen - offset) > 0)) {
    *ptr = &(s->rbuf[offset]);
    return 1;
  }
  return -1;
}
	    
extern int
proto_session_body_marshall_bytes(Proto_Session *s, int len, char *data)
{
  if (s && ((s->slen + len) < PROTO_SESSION_BUF_SIZE)) {
    memcpy(s->sbuf + s->slen, data, len);
    s->slen += len;
    return 1;
  }
  return -1;
}

extern int
proto_session_body_unmarshall_bytes(Proto_Session *s, int offset, int len, 
				     char *data)
{
  if (s && ((s->rlen - (offset + len)) >= 0)) {
    memcpy(data, s->rbuf + offset, len);
    return offset + len;
  }
  return -1;
}

// rc < 0 on comm failures
// rc == 1 indicates comm success
extern  int
proto_session_send_msg(Proto_Session *s, int reset)
{
  s->shdr.blen = htonl(s->slen);

  // write request
  // Write request to protosessions fd. First serialize the header into it, then the body
  // We could do this explicitly as in. Send each field in the header and then the body
  
  // ADD CODE
  // Writing Header
  if (net_writen( s->fd, &(s->shdr), sizeof( s->shdr )) != sizeof( s->shdr) ) return 0;

  // Write the Body
  if (net_writen( s->fd, (s->sbuf) , s->slen )!= s->slen ) return 0;
 
  if (proto_debug()) {
    fprintf(stderr, "%p: proto_session_send_msg: SENT:\n", (void *) pthread_self());
    proto_session_dump(s);
  }

  // communication was successfull 
  if (reset) proto_session_reset_send(s);

  return 1;
}

extern int
proto_session_rcv_msg(Proto_Session *s)
{
  
  proto_session_reset_receive(s);

  // read reply
  // Read Header the Protosession recv header
  // Read the message into the protosession recv buffer
  // Don't unmarshall header, that can be done later by the protoclient/server
  // Because only they now what to do from reading the header

  // ADD CODE
  // Read Header
  if (net_readn( s->fd, &(s->rhdr) , sizeof( s->rhdr)) != sizeof(s->rhdr) ) return 0;

  // Set the RLEN
  s->rlen = proto_session_hdr_unmarshall_blen(s);

  // Read Body
  if (net_readn( s->fd, &(s->rbuf) , s->rlen ) != s->rlen ) return 0;

  if (proto_debug()) {
    fprintf(stderr, "%p: proto_session_rcv_msg: RCVED:\n", (void *) pthread_self());
    proto_session_dump(s);
  }
  return 1;
}

// According to Professor Jappavoo
// This function is supposed to be a wrapper 
// that sends and receives in a single function call.
extern int
proto_session_rpc(Proto_Session *s)
{
  int rc;
  
  //ADD CODE
  //Send Message
  proto_session_send_msg(s,1);

  //Recv Message
  proto_session_rcv_msg(s);
 
  // Pull result 
  proto_session_body_unmarshall_int( s, 0, &rc);

  return rc;
}

