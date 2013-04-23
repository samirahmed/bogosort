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

/*  update_players 
    Takes the player compress array and decompresses
    each element and adds the information to the Maze.

    parameter: num_elements    the number of elements in the player compress array
    parameter: player_compress pointer to the array that contains player compresses
    parameter: maze            pointer to client version of the maze
    return:    void
*/
void update_players(int num_elements,int* player_compress, Maze* maze)
{
    int ii,x,y;
    Player player;
    Player* player_ptr;
    Player_Update_Types update_type;
    for(ii = 0; ii < num_elements; ii++)
    {
        if(!decompress_is_ignoreable(&player_compress[ii])) 
        {
            decompress_player(&player,&player_compress[ii],&update_type);

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

/*  update_objects 
    Take the data in the object compress array and decompresses
    each element and adds the information to the Maze.

    parameter: num_elements    the number of elements in the player compress array
    parameter: object_compress pointer to the array that contains object compresses
    parameter: maze            pointer to client version of the maze
    return:    void
*/
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


/*  update_walls
    Takes game compress array and decompresses
    each element and adds broken wall information to the Maze.

    parameter: num_elements    the number of elements in the player compress array
    parameter: game_compress   pointer to the array that contains game compresses
    parameter: maze            pointer to client version of the maze
    return:    void
*/
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


/*  request_action_init
    Fills the request data structure with a
    - pointer to a client,
    - type of action that will be sent in the request (e.g., ACTION_MOVE)
    - current position of the player                  (iff non-null)
    - next position of the player                     (iff non-null)
    - the Proto Message type of the request  (as specified in protocol.h)
    
    parameter: request         pointer to a request data structure
    parameter: client          pointer to a client data structure
    parameter: action          action type of the request
    parameter: current         pointer client's current position data structure
    parameter: next            pointer to client's next position data structure
    return:    void
*/
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


/*  request_hello_init
    Fills the request data structure with a
    - pointer to a client,
    - the Proto Message type of the request  (as specified in protocol.h)
    
    parameter: request         pointer to a request data structure
    parameter: client          pointer to a client data structure
    return:    void
*/
void request_hello_init(Request* request,Client* client)
{
    bzero(request,sizeof(Request));
    request->client = client;
    request->type = PROTO_MT_REQ_HELLO;
}


/*  request_goodbye_init
    Fills the request data structure with a
    - pointer to a client,
    - the Proto Message type of the request  (as specified in protocol.h)
    
    parameter: request         pointer to a request data structure
    parameter: client          pointer to a client data structure
    return:    void
*/
void request_goodbye_init(Request* request,Client* client)
{
    bzero(request,sizeof(Request));
    request->client = client;
    request->type = PROTO_MT_REQ_GOODBYE;
}

/*  request_sync_init
    Fills the request data structure with a
    - pointer to a client,
    - the Proto Message type of the request  (as specified in protocol.h)
    
    parameter: request         pointer to a request data structure
    parameter: client          pointer to a client data structure
    return:    void
*/
void request_sync_init(Request* request,Client* client)
{
    bzero(request,sizeof(Request));
    request->client = client;
    request->type = PROTO_MT_REQ_SYNC;
}

/*  process_hello_request
    After the RPC thread recieves a hello response from the server
    we set the player's if and team based on the information sent
    by the server in the Proto_Msg_Hdr
   
    parameter: maze         pointer to the client's local version of the maze 
    parameter: hdr          Proto_Msg_Hdr that was unmarshalled from the server's response
    parameter: my_player    pointer to my player structure
    return:    int          return code
*/
int process_hello_request(Maze* maze, Player* my_player, Proto_Msg_Hdr* hdr)
{

   //Get the team and player id from the header
   Team_Types team = hdr->pstate.v1.raw;
   int id = hdr->pstate.v0.raw;

   //Set local Client player pointer to corresponding player in the plist
   my_player = &(maze->players[team].at[id]);

   my_player->id = id;
   my_player->team = team;


    return hdr->gstate.v0.raw;
}

/*  process_goodbye_request
    After the RPC thread recieves a goodbye response from the server
    we set the player's if and team based on the information sent
    by the server in the Proto_Msg_Hdr
   
    parameter: hdr          Proto_Msg_Hdr that was unmarshalled from the server's response
    return:    int          return code
*/
int process_goodbye_request(Proto_Msg_Hdr* hdr)
{
    return hdr->gstate.v0.raw;
}

/*  process_action_request
    After the RPC thread recieves a goodbye response from the server
    we set the player's if and team based on the information sent
    by the server in the Proto_Msg_Hdr
   
    parameter: my_player    pointer to my player structure
    parameter: ch           handle to a Proto_Client
    return:    int          return code
*/
int process_action_request(Player* my_player, Proto_Client_Handle ch)
{
    int result;
    get_int(ch,0,&result);
    return result;
}

/*  process_sync_request
    After the RPC thread recieves a sync response from the server
    we update the object information in the maze, the broken
    walls in the maze and the players in the game
   
    parameter: maze         pointer to the client's local copy of the maze
    parameter: ch           handle to a Proto_Client
    parameter: hdr          pointer to hdr that contains unmarshalled data
    return:    int          return code
*/
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
    broken_walls_compress = (int*) malloc(num_walls*sizeof(int));
    player_compress = (int*) malloc(num_players*sizeof(int));
    object_compress = (int*) malloc(4*sizeof(int));

    //Get the data from the body of the message
    offset = 0;
    offset = get_compress_from_body(ch, offset, num_walls, broken_walls_compress);
    offset = get_compress_from_body(ch, offset, num_players, player_compress);
    offset = get_compress_from_body(ch, offset, 4, object_compress);

    update_objects(4,object_compress,maze);
    update_walls(num_walls,broken_walls_compress,maze);
    update_players(num_players,player_compress,maze);


    //De-allocate the malloced variables
    free(broken_walls_compress);
    free(player_compress);
    free(object_compress);
    return hdr->gstate.v0.raw;    
}

/*  process_RPC_message
    After the RPC thread recieves a response from the server
    this function will call one of the functions to process the data 
    received
   
    parameter: C            pointer client data structure
    return:    int          return code from the header
*/
int process_RPC_message(Client *C)
{
    Proto_Msg_Hdr hdr;
    get_hdr(C->ph,&hdr);
    int rc;
    
    switch(hdr.type)
    {
        case PROTO_MT_REP_HELLO:
            rc = process_hello_request(&C->maze,C->my_player,&hdr);
            break;
        case PROTO_MT_REQ_GOODBYE:
            rc = process_goodbye_request(&hdr);
            break;
        case PROTO_MT_REP_ACTION:
            rc = process_action_request(C->my_player,C->ph);
            break;
        case PROTO_MT_REP_SYNC:
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

/*  client_init
    Initialize the client data structure
   
    parameter: C            pointer client data structure
    return:    int          1 or -1 for success or failure respectively
*/
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

/*  client_map_init
    Load the maze with the information in .map files
   
    parameter: C            pointer client data structure
    parameter: filename     filename for the map file
    return:    int          1 or -1 for success or failure respectively
*/
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

/*  doRPCCmd
    Based on the request type (set by one of the init function)
    send an RPC to the server
   
    parameter: request     pointer to request data structure that was already
                            initialized
    return:    int          1 or -1 for success or failure respectively
*/
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
    rc = do_no_body_rpc(C->ph,&hdr);
    break;
  case PROTO_MT_REQ_GOODBYE:
    fprintf(stderr,"Goodbye COMMAND ISSUED");
    hdr.type = request->type;
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

/*  blocking_helper_init
    Initialize the blocking helper data structure
   
    parameter: request     pointer to allocated Blocking_Helper 
    return:    int         1 or -1 for success or failure respectively
*/
extern int blocking_helper_init(Blocking_Helper *bh)
{
    bzero(bh,sizeof(Blocking_Helper));
    if(pthread_mutex_init(&bh->maze_lock,NULL)!=0)
        return -1;
    if(pthread_cond_init(&bh->maze_updated,NULL)!=0)
        return -1;
    return 1;
}

/*  blocking_helper_set_maze
    have the blocking helper's maze parameter point to the local
    copy of the maze
   
    parameter: bh     pointer to allocated Blocking_Helper 
    parameter: m      pointer to local copy of the maze 
    return:    void
*/
extern void blocking_helper_set_maze(Blocking_Helper *bh, Maze* m)
{
    bh->maze = m;    
}

/*  blocking_helper_destroy
    destroy the mutex and the condition variable
   
    parameter: bh     pointer to allocated Blocking_Helper 
    return:    int    returns 1 or -1 for success or failure respectively
*/
extern int blocking_helper_destroy(Blocking_Helper *bh)
{
    if(pthread_mutex_destroy(&bh->maze_lock)!=0)
        return -1;
    if(pthread_cond_destroy(&bh->maze_updated)!=0)
        return -1;
   return 1;
}




/*  client_maze_lock
    locks the mutex in the Blocking_Helper
   
    parameter: bh     pointer to allocated Blocking_Helper 
    return:    void
*/
extern int client_maze_lock(Blocking_Helper *bh)
{
    return pthread_mutex_lock(&bh->maze_lock);
}

/*  client_maze_unlock
    unlocks the mutex in the Blocking_Helper
   
    parameter: bh     pointer to allocated Blocking_Helper 
    return:    void
*/
extern int client_maze_unlock(Blocking_Helper *bh)
{
    return pthread_mutex_unlock(&bh->maze_lock);
}

/*  client_maze_signal
    signals the condition variable in the Blocking_Helper
   
    parameter: bh     pointer to allocated Blocking_Helper 
    return:    void
*/
extern int client_maze_signal(Blocking_Helper *bh)
{
    return pthread_cond_signal(&bh->maze_updated);
}

/*  client_maze_cond_wait
    waits for the condition variable to be signaled in the Blocking_Helper
   
    parameter: bh     pointer to allocated Blocking_Helper 
    return:    void
*/
extern int client_maze_cond_wait(Blocking_Helper *bh)
{
    return pthread_cond_wait(&bh->maze_updated,&bh->maze_lock);
}
