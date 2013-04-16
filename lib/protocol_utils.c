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
#include <arpa/inet.h>
#include <sys/types.h>
#include <strings.h>
#include <errno.h>

#include "protocol.h"
#include "protocol_utils.h"

int PROTO_DEBUG=0;

extern void
cell_dump(Cell *cell)
{ 
  fprintf(stderr, "\n");
  fprintf(stderr, "COLUMN - (x) - %d \n",cell->pos.x);
  fprintf(stderr, "ROW    - (y) - %d \n",cell->pos.y);

  switch(cell->type)
  {  
    case CELL_WALL:
      fprintf(stderr, "CELL TYPE: CELL_WALL");
      break;
    case CELL_FLOOR:
      fprintf(stderr, "CELL TYPE: CELL_FLOOR");
      break;
    case CELL_HOME:
      fprintf(stderr, "CELL TYPE: CELL_HOME");
      break;
    case CELL_JAIL:
      fprintf(stderr, "CELL TYPE: CELL_JAIL");
      break;
    default:
      fprintf(stderr, "CELL TYPE: UNKWOWN");
      break;
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "CELL TURF: %d",cell->turf+1);
  fprintf(stderr, "\n");
  
  switch(cell->cell_state)
  {  
    case CELLSTATE_EMPTY:
      fprintf(stderr, "CELLSTATE: EMPTY");
      break;
    case CELLSTATE_HOLDING:
      fprintf(stderr, "CELLSTATE: HOLDING");
      /*fprintf(stderr, "PLAYER OBJECT_TYPE %d \n",cell->object_type);*/
      break;
    case CELLSTATE_OCCUPIED:
      fprintf(stderr, "CELLSTATE: OCCUPIED\n");
      /*fprintf(stderr, "PLAYER TEAM %d \n",cell->player_type+1);*/
      break;
    case CELLSTATE_OCCUPIED_HOLDING:
      fprintf(stderr, "CELLSTATE: OCCUPIED AND HOLDING");
      /*fprintf(stderr, "PLAYER TEAM %d \n",cell->player_type+1);*/
      /*fprintf(stderr, "PLAYER OBJECT_TYPE %d \n",cell->object_type);*/
      break;
    default:
      fprintf(stderr, "CELLSTATE: UNKWOWN");
      break;
  }
  fprintf(stderr, "\n");
}

extern void
proto_dump_mt(Proto_Msg_Types type)
{
  switch (type) {
  case PROTO_MT_REQ_BASE_RESERVED_FIRST: 
    fprintf(stderr, "PROTO_MT_REQ_BASE_RESERVED_FIRST");
    break;
  case PROTO_MT_REQ_HELLO:
    fprintf(stderr, "PROTO_MT_REQ_BASE_HELLO");
    break;
  case PROTO_MT_REQ_ACTION: 
    fprintf(stderr, "PROTO_MT_REQ_ACTION");
    break;
  case PROTO_MT_REQ_SYNC: 
    fprintf(stderr, "PROTO_MT_REQ_SYNC");
    break;
  case PROTO_MT_REQ_GOODBYE: 
    fprintf(stderr, "PROTO_MT_REQ_BASE_GOODBYE");
    break;
  case PROTO_MT_REQ_BASE_RESERVED_LAST: 
    fprintf(stderr, "PROTO_MT_REQ_BASE_RESERVED_LAST");
    break;
  case PROTO_MT_REP_BASE_RESERVED_FIRST: 
    fprintf(stderr, "PROTO_MT_REP_BASE_RESERVED_FIRST");
    break;
  case PROTO_MT_REP_HELLO: 
    fprintf(stderr, "PROTO_MT_REP_BASE_HELLO");
    break;
  case PROTO_MT_REP_ACTION:
    fprintf(stderr, "PROTO_MT_REP_ACTION");
    break;
  case PROTO_MT_REP_SYNC:
    fprintf(stderr, "PROTO_MT_REP_ACTION");
    break;
  case PROTO_MT_REP_GOODBYE:
    fprintf(stderr, "PROTO_MT_REP_BASE_GOODBYE");
    break;
  case PROTO_MT_REP_BASE_RESERVED_LAST: 
    fprintf(stderr, "PROTO_MT_REP_BASE_RESERVED_LAST");
    break;
  case PROTO_MT_EVENT_BASE_RESERVED_FIRST: 
    fprintf(stderr, "PROTO_MT_EVENT_BASE_RESERVED_LAST");
    break;
  case PROTO_MT_EVENT_UPDATE: 
    fprintf(stderr, "PROTO_MT_EVENT_BASE_UPDATE");
    break;
  case PROTO_MT_EVENT_BASE_RESERVED_LAST: 
    fprintf(stderr, "PROTO_MT_EVENT_BASE_RESERVED_LAST");
    break;
  default:
    fprintf(stderr, "UNKNOWN=%d", type);
  }
}
 
extern void
proto_dump_pstate(Proto_Player_State *ps)
{
  int v0, v1, v2, v3;
  
  v0 = ntohl(ps->v0.raw);
  v1 = ntohl(ps->v1.raw);
  v2 = ntohl(ps->v2.raw);
  v3 = ntohl(ps->v3.raw);

  fprintf(stderr, "v0=x0%x v1=0x%x v2=0x%x v3=0x%x\n",
	  v0, v1, v2, v3);
}

extern void
proto_dump_gstate(Proto_Game_State *gs)
{
  int v0, v1, v2;

  v0 = ntohl(gs->v0.raw);
  v1 = ntohl(gs->v1.raw);
  v2 = ntohl(gs->v2.raw);

  fprintf(stderr, "v0=0x%x v1=0x%x v2=0x%x\n",
	  v0, v1, v2);
}

extern void
proto_dump_msghdr(Proto_Msg_Hdr *hdr)
{
  fprintf(stderr, "ver=%d type=", ntohl(hdr->version));
  proto_dump_mt(ntohl(hdr->type));
  fprintf(stderr, " sver=%llx", ntohll(hdr->sver.raw));
  fprintf(stderr, " pstate:");
  proto_dump_pstate(&(hdr->pstate));
  fprintf(stderr, " gstate:"); 
  proto_dump_gstate(&(hdr->gstate));
  fprintf(stderr, " blen=%d\n", ntohl(hdr->blen));
}
