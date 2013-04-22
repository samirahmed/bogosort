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
#include <unistd.h>
#include "client.h"
#include "../lib/types.h"
#include "../lib/protocol.h"
#include "../lib/protocol_client.h"
#include "../lib/protocol_utils.h"
#include "../lib/game_client.h"

//Global Variables
Player* my_player;      //Pointer to client's player struct
Globals globals;         //Host string and port
static int connected;
static char MenuString[] = "\n?> ";

void update_players(int num_elements,int* player_compress, Maze* maze)
{
    int ii,x,y;
    Player player;
    Player* player_ptr;
    for(ii = 0; ii < num_elements; ii++)
    {
        if(!decompress_is_ignoreable(&player_compress[ii])) 
        {
            decompress_player(&player,&player_compress[ii],NULL);

           player_ptr =  &(maze->players[player.team].at[player.id]);
           bzero(player_ptr,sizeof(Player));            
           
           x = player.client_position.x;
           y = player.client_position.y; 

           //Fill in the player data structure TODO: is all this information needed?
           player_ptr->client_position.x = x;
           player_ptr->client_position.y = y;
           player_ptr->id = player.id;
           player_ptr->state = player.state;
           player_ptr->team = player.team;

           //Set the player pointer at cell position x,y to my player
           maze->get[x][y].player = player_ptr; 
        }
    }
}

void update_objects(int num_elements,int* object_compress, Maze* maze)
{
    int ii,x,y;
    Object object;
    Object* object_ptr;
    Player* player;
    Cell* cell;
    for(ii = 0; ii < num_elements; ii++)
    {
        if(!decompress_is_ignoreable(&object_compress[ii])) 
        {
            decompress_object(&object,&object_compress[ii]);
            x = object.client_position.x;
            y = object.client_position.y;
            object_ptr = &maze->objects[object_get_index(object.type,object.team)];
            object_ptr->client_position.x = x;
            object_ptr->client_position.y = y;
            object_ptr->team = object.team;
            object_ptr->type = object.type;

            if(object.client_has_player)
            {
                player = &maze->players[object.client_player_team].at[object.client_player_id];
                if(object.type==OBJECT_SHOVEL)
                    player->shovel = object_ptr;
                else
                    player->flag = object_ptr;
            }
            else{
                cell = &maze->get[x][y];
                cell->object = object_ptr;
            }

        }
    }
    
}

void update_walls(int num_elements,int* game_compress, Maze* maze)
{
    Pos pos;
    int ii,x,y;
    for(ii = 0; ii < num_elements; ii++)
    {
        if(!decompress_is_ignoreable(&game_compress[ii])) 
        {
            decompress_broken_wall(&pos,&game_compress[ii]);
           x = pos.x;
           y = pos.y; 
           maze->get[x][y].type = CELL_FLOOR;
        }
    }
    
}


void request_action_init(Request* request, Client* client,Action_Types action,Pos* current, Pos* next)
{
    bzero(request,sizeof(Request));
    request->client = client;
    request->current  = *current;
    request->type = PROTO_MT_REQ_ACTION;
    request->action_type = action;
    if(current)
        request->current = *current;
    if(next)
        request->next = *next;
    if(action==ACTION_MOVE)
        request->next = *next;
}

void request_hello_init(Request* request,Client* client)
{
    bzero(request,sizeof(Request));
    request->client = client;
    request->type = PROTO_MT_REQ_HELLO;
}

void request_goodbye_init(Request* request,Client* client)
{
    bzero(request,sizeof(Request));
    request->client = client;
    request->type = PROTO_MT_REQ_GOODBYE;
}

void request_sync_init(Request* request,Client* client)
{
    bzero(request,sizeof(Request));
    request->client = client;
    request->type = PROTO_MT_REQ_SYNC;
}

int process_hello_request(Maze* maze, Player* my_player, Proto_Client_Handle ch, Proto_Msg_Hdr* hdr)
{
   //Get the Position of the Player
   Pos current;
   get_pos(ch,&current);

   //Get the team and player id from the header
   Team_Types team = hdr->pstate.v1.raw;
   int id = hdr->pstate.v0.raw;

   //Set local Client player pointer to corresponding player in the plist
   my_player = &(maze->players[team].at[id]);

   //Fill in the player data structure TODO: is all this information needed?
   my_player->client_position.x = current.x;
   my_player->client_position.y = current.y;
   my_player->id = id;
   my_player->state = PLAYER_FREE;
   my_player->team = team;

   //Set the player pointer at cell position x,y to my player
   maze->get[current.x][current.y].player = &(maze->players[team].at[id]); 

    return hdr->gstate.v0.raw;
}

int process_goodbye_request(Proto_Client_Handle ch, Proto_Msg_Hdr* hdr)
{
    return hdr->gstate.v0.raw;
}
int process_action_request(Player* my_player, Proto_Client_Handle ch)
{
    int result;
    get_int(ch,0,&result);
    return result;
}
int process_sync_request(Maze* maze, Proto_Client_Handle ch, Proto_Msg_Hdr* hdr)
{
    // Variable Declaration
    int* broken_walls_compress;
    int* player_compress;
    int* object_compress;
    int offset;

    //Get number of elements for walls and players
    int num_walls = hdr->pstate.v2.raw;
    int num_players = hdr->pstate.v3.raw;

    //Malloc the variables
    broken_walls_compress = (int*) malloc(num_walls);
    player_compress = (int*) malloc(num_players);
    object_compress = (int*) malloc(4);

    //Get the data from the body of the message
    offset = 0;
    offset = get_compress_from_body(ch, offset, num_walls, broken_walls_compress);
    offset = get_compress_from_body(ch, offset, num_players, player_compress);
    offset = get_compress_from_body(ch, offset, 4, object_compress);

    //De-allocate the malloced variables
    free(broken_walls_compress);
    free(player_compress);
    free(object_compress);
    return hdr->gstate.v0.raw;    
}

int process_RPC_message(Client *C)
{
    Proto_Msg_Hdr hdr;
    get_hdr(C->ph,&hdr);
    int rc;
    
    switch(hdr.type)
    {
        case PROTO_MT_REP_HELLO:
            rc = process_hello_request(&C->maze,C->my_player,C->ph,&hdr);
            break;
        case PROTO_MT_REQ_GOODBYE:
            rc = process_goodbye_request(C->ph,&hdr);
            break;
        case PROTO_MT_REQ_ACTION:
            rc = process_action_request(C->my_player,C->ph);
            break;
        case PROTO_MT_REQ_SYNC:
            rc =process_sync_request(&C->maze,C->ph,&hdr);
            break;
        default:
            return -1;
    }
    if(rc>0)
        return hdr.type;
    else
        return rc;
}


static int update_handler(Proto_Session *s )
{
    return 0;
}

static int client_init(Client *C)
{
  // Zero global scope
  bzero(C, sizeof(Client));

  // Set connected state to zero
  connected = 0; 
  
  // initialize the client protocol subsystem
  if (proto_client_init(&(C->ph))<0) {
    fprintf(stderr, "client: main: ERROR initializing proto system\n");
    return -1;
  }
 
  // Specify the event channel handlers
  proto_client_set_event_handler(C->ph, PROTO_MT_EVENT_UPDATE, update_handler );
  return 1;
}

int client_map_init(Client *C,char* filename)
{
    //Build maze from file
    if(maze_build_from_file(&C->maze,filename)==-1)
        return -1;

    //Initialize the Blocking Data Structure
    if(blocking_helper_init(&C->bh)==-1)
        return -1;

    //Set the maze pointer in the blocking helper
    blocking_helper_set_maze(&C->bh,&C->maze);
    return 1;
}



static int update_event_handler(Proto_Session *s)
{
  /*Client *C = proto_session_get_data(s);*/

  fprintf(stderr, "%s: called", __func__);
  return 1;
}


int startConnection(Client *C, char *host, PortType port, Proto_MT_Handler h)
{
  if (globals.host[0]!=0 && globals.port!=0) {
    int res;
  if (( res = proto_client_connect(C->ph, host, port))!=0) {
      fprintf(stderr, "failed to connect\n : Error code %d",res);
      return -1;
    }
    proto_session_set_data(proto_client_event_session(C->ph), C);
#if 0
    if (h != NULL) {
      proto_client_set_event_handler(C->ph, PROTO_MT_EVENT_BASE_UPDATE, 
             h);
    }
#endif
    return 1;
  }
  return 0;
}

char* prompt(int menu) 
{
  if (menu) printf("%s", MenuString);
  fflush(stdout);
  
  // Pull in input from stdin
  int bytes_read;
  size_t nbytes = 0;
  char *my_string;
  bytes_read = getline(&my_string, &nbytes, stdin);

  if(bytes_read>0)
    return my_string;
  else
    return 0;

}

int doRPCCmd(Request* request) 
{
  int rc=-1;
 
  // Unpack the request
  Client *C;
  Proto_Msg_Hdr hdr;
  bzero(&hdr,sizeof(Proto_Msg_Hdr));

  C = request->client;

  switch (request->type) {
  case PROTO_MT_REQ_HELLO:  
    fprintf(stderr,"HELLO COMMAND ISSUED");
    hdr.type = request->type;
    rc = do_no_body_rpc(C->ph,&hdr);
    if (proto_debug()) fprintf(stderr,"hello: rc=%x\n", rc);
    if (rc < 0) fprintf(stderr, "Unable to connect");
    break;
  case PROTO_MT_REQ_ACTION:
    fprintf(stderr,"Action COMMAND ISSUED");
    hdr.type = request->type;
    hdr.gstate.v1.raw = request->action_type;
    hdr.pstate.v0.raw = my_player->id;
    rc = do_action_request_rpc(C->ph,&hdr,request->current,request->next);
    break;
  case PROTO_MT_REQ_SYNC:
    fprintf(stderr,"Sync COMMAND ISSUED");
    hdr.type = request->type;
    hdr.pstate.v0.raw = my_player->id;
    rc = do_no_body_rpc(C->ph,&hdr);
    break;
  case PROTO_MT_REQ_GOODBYE:
    fprintf(stderr,"Goodbye COMMAND ISSUED");
    hdr.type = request->type;
    hdr.pstate.v0.raw = C->my_player->id;
    rc = do_no_body_rpc(C->ph,&hdr);
    /*rc = proto_client_goodbye(C->ph);*/
    /*printf("Game Over - You Quit");*/
    break;
  default:
    printf("%s: unknown command %d\n", __func__, request->type);
  }
  // NULL MT OVERRIDE ;-)
  if(proto_debug()) fprintf(stderr,"%s: rc=0x%x\n", __func__, rc);
  if (rc == 0xdeadbeef) rc=1;
  printf("rc=1\n");
  return rc;
}

void disconnect (Client *C)
{
  connected = 0;
  Proto_Session *event = proto_client_event_session(C->ph);
  Proto_Session *rpc = proto_client_rpc_session(C->ph);
  close(event->fd);
  close(rpc->fd);
}

int doConnect(Client *C, char* cmd)
{
  int rc;
  Request request;
  bzero(&request,sizeof(Request));
  char address[2][STRLEN]; 

  char* token;
  token = strtok(cmd+8,":i\n\0"); //Tokenize C-String for ip
  if (token == NULL) 
  {
    fprintf(stderr, "Please specify as such >connect <host>:<port>"); 
    return -1;
  }
  strcpy(address[0],token);   //Put ip address into address

  token = strtok(NULL,":i\n\0"); //Tokenize C-String for port
  if (token == NULL) 
  {
    fprintf(stderr,"Please specify as such >connect <host>:<port>"); 
    return -1;
  }
  strcpy(address[1],token);   //put port number into address

  globals_init(2,address);

  // ok startup our connection to the server
  connected = 1;      //Change connected state to true
  if (startConnection(C, globals.host, globals.port, update_event_handler)<0) 
  {
    fprintf(stderr, "ERROR: Not able to connect to %s:%d\n",globals.host,(int)globals.port);
    connected =0;
    return -2;
  }

  // configure request parameters
  request.type = PROTO_MT_REQ_HELLO;
  rc = doRPCCmd(&request);

  return rc;
}

int docmd(Client *C, char* cmd)
{
  int rc = 1;                      // Set up return code var

  if(strncmp(cmd,"quit",sizeof("quit")-1)==0) return -2;

  if(!connected && strncmp(cmd,"connect",sizeof("connect")-1)==0)
  {
    rc = doConnect(C, cmd);
  }
  else if(strncmp(cmd,"where",sizeof("where")-1)==0)
  {
    if (connected) printf("Host = %s : Port = %d", globals.host , (int) globals.port );
    else printf("Not connected\n");
  }
  else if( connected )
  {
    Request request;

    if( strncmp(cmd,"disconnect",sizeof("disconnect")-1)==0)
    {  
        request_goodbye_init(&request,C);
        rc=doRPCCmd(&request);
        disconnect(C);
    }
    else if(strncmp(cmd,"move",sizeof("move")-1)==0)
    {
        char* pch;
        Pos next;
        pch = strtok(cmd+5," ");
        next.x = atoi(pch);
        pch = strtok(NULL," ");
        next.y = atoi(pch);
        request_action_init(&request,C,ACTION_MOVE,&my_player->client_position,&next);
        rc = doRPCCmd(&request);

    }
    else if(strncmp(cmd,"pickup flag",sizeof("pickup flag")-1)==0)
    {
        request_action_init(&request,C,ACTION_PICKUP_FLAG,NULL,NULL);
        rc = doRPCCmd(&request);

    }
    else if(strncmp(cmd,"drop flag",sizeof("drop flag")-1)==0)
    {
        request_action_init(&request,C,ACTION_DROP_FLAG,NULL,NULL);
        rc = doRPCCmd(&request);
    }
    else if(strncmp(cmd,"pickup shovel",sizeof("pickup shovel")-1)==0)
    {
        request_action_init(&request,C,ACTION_PICKUP_SHOVEL,NULL,NULL);
        rc = doRPCCmd(&request);

    }
    else if(strncmp(cmd,"drop shovel",sizeof("drop shovel")-1)==0)
    {
        request_action_init(&request,C,ACTION_DROP_SHOVEL,NULL,NULL);
        rc = doRPCCmd(&request);
    }
    else if(strncmp(cmd,"sync",sizeof("drop shovel")-1)==0)
    {
        request_sync_init(&request,C);
        rc = doRPCCmd(&request);
    }
    else if(strncmp(cmd,"load",sizeof("load")-1)==0)
    {
        char* pch;
        Pos next;
        pch = strtok(cmd+5," \n\0");
        client_map_init(C,pch);
    }
  }
  else
  {
    printf("Please connect first i.e 'connect <host>:<port>'\n");
  }

  return rc;
}

void* shell(void *arg)
{
  Client *C = arg;
  char *c;
  int rc;
  int menu=1;

  while (1) {
    if ((c = prompt(menu))!=0) rc=docmd(C, c);
    if (rc == -2) break; //only terminate when client issues 'q'
  
  //If this variable was allocated in prompt(menu) please free memory
  if(c!=0) free(c);
  }
  
  fprintf(stderr, "terminating\n");
  fflush(stdout);
  return NULL;
}

void usage(char *pgm)
{
  fprintf(stderr, "USAGE: %s <port|<<host port> [shell] [gui]>>\n"
           "  port     : rpc port of a game server if this is only argument\n"
           "             specified then host will default to localhost and\n"
         "             only the graphical user interface will be started\n"
           "  host port: if both host and port are specifed then the game\n"
         "examples:\n" 
           " %s 12345 : starts client connecting to localhost:12345\n"
         " %s localhost 12345 : starts client connecting to locaalhost:12345\n",
     pgm, pgm, pgm);
 
}

void globals_init(int argc, char argv[][STRLEN])
{
  bzero(&globals, sizeof(globals));

  if (argc==1) {
    usage(argv[0]);
    exit(-1);
  }

  if (argc==2) {
    strncpy(globals.host, argv[0], STRLEN);
    globals.port = atoi(argv[1]);
  }

  if (argc>=3) {
    strncpy(globals.host, argv[1], STRLEN);
    globals.port = atoi(argv[2]);
  }
  
}

int main(int argc, char **argv)
{
  Client c;
  
  if (client_init(&c) < 0) {
    fprintf(stderr, "ERROR: clientInit failed\n");
    return -1;
  }    

  shell(&c);

  return 0;
}
