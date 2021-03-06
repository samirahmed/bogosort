#ifndef __DAGAME_PROTOCOL_CLIENT_H__
#define __DAGAME_PROTOCOL_CLIENT_H__
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

#include "net.h"
#include "protocol.h"
#include "protocol_session.h"
#include "types.h"
#include "game_commons.h"

typedef void * Proto_Client_Handle;

extern Proto_Session *proto_client_rpc_session(Proto_Client_Handle ch);
extern Proto_Session *proto_client_event_session(Proto_Client_Handle ch);

extern int proto_client_init(Proto_Client_Handle *ch);
extern int proto_client_connect(Proto_Client_Handle ch, char *host, PortType p);

extern int proto_client_set_session_lost_handler(Proto_Client_Handle ch,
						 Proto_MT_Handler h);

extern int proto_client_set_event_handler(Proto_Client_Handle ch,
					  Proto_Msg_Types mt,
					  Proto_MT_Handler h);

// client side protocol rpc's
extern int get_int(Proto_Client_Handle ch, int offset, int* result);
extern void get_hdr(Proto_Client_Handle ch, Proto_Msg_Hdr * hdr);
extern void get_pos(Proto_Client_Handle ch, Pos* current);
extern int get_compress_from_body(Proto_Client_Handle ch,int offset ,int num_elements,int* alloc_pointer);
extern int do_no_body_rpc(Proto_Client_Handle ch, Proto_Msg_Hdr * h);
extern int do_action_request_rpc(Proto_Client_Handle ch, Proto_Msg_Hdr * h,Pos curent,Pos next);
#endif
