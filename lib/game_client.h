#ifndef __CLIENT_MAZE_H__
#define __CLIENT_MAZE_H__
/******************************************************************************
* Copyright (C) 2013 by Katsu Kawakami, Will Seltzer, Samir Ahmed, 
* Boston University
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
#include "./types.h" 
#include "./game_commons.h" 
#include "./protocol_client.h"
#include "./protocol_utils.h"
#include <pthread.h>
#define STRLEN 81


typedef struct ClientBlockingStruct{
    Maze *maze;    
    pthread_mutex_t maze_lock;
    pthread_cond_t maze_updated;
    int RPC_update_id;
    int EC_update_id;
} Blocking_Helper;

typedef struct ClientState  {
  Proto_Client_Handle ph;
  pthread_t UI_Thread;
  Maze maze;
  Blocking_Helper bh;
  Player* my_player;
  int connected;
  uval height;
  uval width;
} Client;

typedef struct RequestHandler{
  Client * client;
  Proto_Msg_Types type;
  Action_Types action_type;
  Pos current;
  Pos next;
}Request;

typedef struct GlobalsInfo {
  char host[STRLEN];
  PortType port;
}Globals;





//Initilization Functions
extern void globals_init(int argc, char argv[][STRLEN]);//Had to Change function header to make this work
extern int client_init(Client *C,Proto_MT_Handler update_handler);
extern int client_map_init(Client *C,char* filename);

//RPC calls based on request information
extern int doRPCCmd(Request* request); //Master RPC CALLER RAWR

//Set Request Types and Required Information
extern void request_action_init(Request* request, Client* client, Action_Types action, Pos* current, Pos* next);
extern void request_hello_init(Request* request, Client* client);
extern void request_goodbye_init(Request* request, Client* client);
extern void request_sync_init(Request* request, Client* client);

//Read proto_session informations
extern int process_RPC_message(Client* c);
extern int process_hello_request(Maze* maze, Player** my_player, Proto_Msg_Hdr* hdr);
extern int process_goodbye_request(Proto_Msg_Hdr* hdr);
extern int process_action_request(Blocking_Helper* bh,Proto_Msg_Hdr* hdr, Proto_Client_Handle ch);
extern int process_sync_request(Maze* maze, Proto_Client_Handle ch, Proto_Msg_Hdr* hdr);
        
//Updating Maze functions
extern void update_players(int num_elements,int* player_compress,Maze* maze);
extern void update_objects(int num_elements,int* object_compress,Maze* maze);
extern void update_walls(int num_elements,int* game_compress,Maze* maze);


// Init Methods
extern int blocking_helper_init(Blocking_Helper *bh);
extern void blocking_helper_set_maze(Blocking_Helper *bh, Maze *maze);

// Destroy Methods
extern int blocking_helper_destroy(Blocking_Helper *bh);

// Locking and Conditional Variable Helper Methods
extern int client_maze_lock(Blocking_Helper *bh);
extern int client_maze_unlock(Blocking_Helper *bh);
extern int client_maze_signal(Blocking_Helper *bh);
extern int client_maze_cond_wait(Blocking_Helper *bh);

// Logging helper
extern void c_log(int cmd, int action, int rc, clock_t clk);

#endif
