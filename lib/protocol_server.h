#ifndef __DAGAME_PROTOCOL_SERVER_H__
#define __DAGAME_PROTOCOL_SERVER_H__
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

extern int proto_server_init(void);

extern int proto_server_set_session_lost_handler(Proto_MT_Handler h);
extern int proto_server_set_req_handler(Proto_Msg_Types mt, Proto_MT_Handler h);

extern PortType proto_server_rpcport(void);
extern PortType proto_server_listenport(void);
extern Proto_Session *proto_server_event_session(void);
extern int    proto_server_start_rpc_loop(void);

extern void proto_server_post_event(void);

#endif
