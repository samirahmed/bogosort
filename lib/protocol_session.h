#ifndef __DAGAME_PROTOCOL_SESSION_H__
#define __DAGAME_PROTOCOL_SESSION_H__
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

#define PROTO_SESSION_BUF_SIZE (4096 * 16)

// There is some redundancy here in the lengths
// but it should make debugging easier
typedef struct {
  FDType fd;
  void *extra;   // space to attach session specific data
  int slen;
  int rlen;
  Proto_Msg_Hdr shdr;
  Proto_Msg_Hdr rhdr;
  char sbuf[PROTO_SESSION_BUF_SIZE];
  char rbuf[PROTO_SESSION_BUF_SIZE];
} Proto_Session;

// Define Proto_MT_Handler type as a pointer to a function that returns
// an int and takes a Session pointer as an argument
typedef int (*Proto_MT_Handler)(Proto_Session *);

extern void proto_session_dump(Proto_Session *s);

extern void proto_session_init(Proto_Session *s);
extern void proto_session_reset_send(Proto_Session *s);
extern void proto_session_reset_receive(Proto_Session *s);

extern void proto_session_hdr_marshall(Proto_Session *s, Proto_Msg_Hdr *h);
extern Proto_Msg_Types proto_session_hdr_unmarshall_type(Proto_Session *s);
extern void proto_session_hdr_unmarshall(Proto_Session *s, Proto_Msg_Hdr *h);

extern int  proto_session_body_marshall_ll(Proto_Session *s, long long v);
extern int  proto_session_body_unmarshall_ll(Proto_Session *s, int offset, 
					     long long *v);
extern int  proto_session_body_marshall_int(Proto_Session *s, int v);
extern int  proto_session_body_unmarshall_int(Proto_Session *s, int offset, 
					      int *v);
extern int  proto_session_body_marshall_char(Proto_Session *s, char v);
extern int  proto_session_body_unmarshall_char(Proto_Session *s, int offset,
					       char *v);
extern int  proto_session_body_reserve_space(Proto_Session *s, int num, 
					     char **space);
extern int  proto_session_body_marshall_bytes(Proto_Session *s, int num, 
					      char *b);
extern int  proto_session_body_unmarshall_bytes(Proto_Session *s, int offset, 
						int num, char *b);
extern int  proto_session_send_msg(Proto_Session *s, int reset);
extern int  proto_session_rcv_msg(Proto_Session *s);
extern int  proto_session_rpc(Proto_Session *s);

static inline void 
proto_session_set_data(Proto_Session *s, void *data) {
  s->extra = data;
}

static inline void * 
proto_session_get_data(Proto_Session *s) {
  return s->extra;
}


#endif
