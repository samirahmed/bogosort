#ifndef __DAGAME_PROTOCOL_UTILS_H__
#define __DAGAME_PROTOCOL_UTILS_H__
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

extern int PROTO_DEBUG;

extern void proto_dump_mt(Proto_Msg_Types type);
extern void proto_dump_pstate(Proto_Player_State *ps);
extern void proto_dump_gstate(Proto_Game_State *gs);
extern void proto_dump_msghdr(Proto_Msg_Hdr *hdr);

static inline  void proto_debug_on(void) { PROTO_DEBUG = 1; }
static inline void proto_debug_off(void) { PROTO_DEBUG = 0; }
static inline int  proto_debug(void) {return PROTO_DEBUG; }

#endif
