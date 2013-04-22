#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include <pthread.h>
#include <strings.h>
#include "net.h"
#include "protocol.h"
#include "protocol_session.h"
#include "game_client.h"

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

int client_init(Client *C)
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



extern int blocking_helper_init(Blocking_Helper *bh)
{
    bzero(bh,sizeof(Blocking_Helper));
    if(pthread_mutex_init(&bh->maze_lock,NULL)!=0)
        return -1;
    if(pthread_cond_init(&bh->maze_updated,NULL)!=0)
        return -1;
    return 1;
}

extern void blocking_helper_set_maze(Blocking_Helper *bh, Maze* m)
{
    bh->maze = m;    
}

extern int blocking_helper_destroy(Blocking_Helper *bh)
{
    if(pthread_mutex_destroy(&bh->maze_lock)!=0)
        return -1;
    if(pthread_cond_destroy(&bh->maze_updated)!=0)
        return -1;
   return 1;
}

extern void client_maze_lock(Blocking_Helper *bh)
{
    pthread_mutex_lock(&bh->maze_lock);
}

extern void client_maze_unlock(Blocking_Helper *bh)
{
    pthread_mutex_unlock(&bh->maze_lock);
}

extern void client_maze_signal(Blocking_Helper *bh)
{
    pthread_cond_signal(&bh->maze_updated);
}

extern void client_maze_cond_wait(Blocking_Helper *bh)
{
    pthread_cond_wait(&bh->maze_updated,&bh->maze_lock);
}
extern void client_wait_for_event(Blocking_Helper *bh)
{
    //TODO: Not fully implemented, at this point only has test functionality
    printf("Thread wait_function lock\n");
    client_maze_lock(bh);
    printf("Thread wait_function waiting for condition variable\n");
    while (bh->maze->current_game_state !=GAME_STATE_ACTIVE) 
        client_maze_cond_wait(bh);
    printf("Thread wait_function unlocks\n");
    client_maze_unlock(bh);
    pthread_exit(NULL);
}

extern void client_signal_update(Blocking_Helper *bh)
{
    //TODO: Not fully implemented, at this point only has test functionality
    printf("Thread signal_function lock\n");
    client_maze_lock(bh);
    printf("Thread signal_function change game state\n");
    bh->maze->current_game_state = GAME_STATE_ACTIVE;
    printf("Thread signal_function signal to other thread that state has changed\n");
    client_maze_signal(bh);
    printf("Thread signal_function unlock\n");
    client_maze_unlock(bh);
    pthread_exit(NULL);
}
