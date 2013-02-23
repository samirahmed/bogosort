#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

//  HDR : BODY
//  HDR ->  VERSION | TYPE | PSTATE | GSTATE | BLEN 
//     VERSION : int default version is 0
//     TYPE    : int see enum below
//     PSTATE  : <V0,V1,V2,V3>  
//                 V0:    int
//                 V1:    int 
//                 V2:    int   
//                 V3:    int
//     GSTATE   : <V0,V1,V2> 
//                 V0:   int
//                 V1:  int
//                 V2:  int
//     BLEN     : int length of body

#define PROTOCOL_BASE_VERSION 0

typedef enum  {

  // Requests
  PROTO_MT_REQ_BASE_RESERVED_FIRST, 
  PROTO_MT_REQ_BASE_HELLO, // establish connection to server and add player to game
  PROTO_MT_REQ_BASE_MOVE, // attempt to mark square as client who submitted request
  PROTO_MT_REQ_BASE_GOODBYE, //disconnect client from server
  // RESERVED LAST REQ MT PUT ALL NEW REQ MTS ABOVE
  PROTO_MT_REQ_BASE_RESERVED_LAST,
  
  // Replys
  PROTO_MT_REP_BASE_RESERVED_FIRST,
  PROTO_MT_REP_BASE_HELLO, //1  if succesful, 0 if player is already in game, -1 if fails
  PROTO_MT_REP_BASE_MOVE, // 1 if square is empty and was marked succesfully, 0 if square is already full and cannot be marked, -1 if fail
  PROTO_MT_REP_BASE_GOODBYE, // 1 if succesful, -1 if fails
  // RESERVED LAST REP MT PUT ALL NEW REP MTS ABOVE
  PROTO_MT_REP_BASE_RESERVED_LAST,

  // Events  
  PROTO_MT_EVENT_BASE_RESERVED_FIRST,
  PROTO_MT_EVENT_BASE_UPDATE, //game state was updated: 1 if game was won and is over, 0 if another square was marked
  PROTO_MT_EVENT_BASE_RESERVED_LAST

} Proto_Msg_Types;

typedef enum { PROTO_STATE_INVALID_VERSION=0, PROTO_STATE_INITIAL_VERSION=1} Proto_SVERS;

typedef union {
  unsigned long long raw;
} Proto_StateVersion;

typedef union {
  int raw;
} Proto_PV0;

typedef union {
  int raw;
} Proto_PV1;

typedef union {
  int raw;
} Proto_PV2;

typedef union {
  int raw;
} Proto_PV3;

typedef struct {
  //player 1 (X) marking states
  //the player has marked square i if the ith bit 
  // in the integer has been set, else the player has not marked that square
  Proto_PV0    v0;  
  //player 2 (O) marking states
  //same encoding as above
  Proto_PV1    v1;

 //PV2 is client ID
  Proto_PV2    v2;

  Proto_PV3    v3;
} __attribute__((__packed__)) Proto_Player_State;

typedef union {
  int raw;
}  Proto_GV0;

typedef union {
  int raw;
} Proto_GV1;

typedef union {
  int raw;
} Proto_GV2;

typedef struct {
  Proto_GV0       v0;  
 //-1: player has dropped connection
 //0: game hasn't started (none connected)
 //1: player 1's turn
 //2: player 2's turn
 //3: player 1 wins
 //4: player 2 wins
 //5: stalemate
 //6: gameover due to lost connection
  Proto_GV1       v1; 
// id for player 1
  Proto_GV2       v2;
// id for player 2
} __attribute__((__packed__)) Proto_Game_State;

typedef struct {
  int                version;
  Proto_Msg_Types    type;
  Proto_StateVersion sver;
  Proto_Player_State pstate;
  Proto_Game_State   gstate;
  int                blen;
} __attribute__((__packed__)) Proto_Msg_Hdr;

// THE FOLLOWING IS TO MAKE SURE THAT YOU GET 
// 64bit ntohll and htonll defined from your
// host OS... only tested for OSX and LINUX
#ifdef __APPLE__
#  ifndef ntohll
#    include <libkern/OSByteOrder.h>
#    define ntohll(x) OSSwapBigToHostInt64(x)
#    define htonll(x) OSSwapHostToBigInt64(x)
#  endif
#else
#  ifndef ntohll
#    include <asm/byteorder.h>
#    define ntohll(x) __be64_to_cpu(x)
#    define htonll(x) __cpu_to_be64(x)
#  endif
#endif

#endif
