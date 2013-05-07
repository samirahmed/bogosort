#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include <pthread.h>
#include <strings.h>
#include <time.h>
#include "net.h"
#include "protocol.h"
#include "protocol_session.h"
#include "game_client.h"

/*  client logging helper
*/
extern void c_log(int cmd, int action, int rc, clock_t clk)
{
  time_t raw;
  int sec;
  struct tm * tt;
  char timestr[9];  
  char rcstr[25];
  
  time(&raw); 
  tt = localtime(&raw);
  sprintf(timestr,"%.2d:%.2d:%.2d",tt->tm_hour,tt->tm_min,tt->tm_sec);
  
  if (rc >= 0)
    sprintf(rcstr,COLOR_OKGREEN "%d" COLOR_END , rc); 
  else
    sprintf(rcstr,COLOR_FAIL "%d" COLOR_END , rc);
  
  sec = (int)(((float)(clock()-clk)/CLOCKS_PER_SEC)*1000);

  char * cmdstr;
  switch (cmd)
  {
      case PROTO_MT_EVENT_UPDATE: cmdstr = "UPD" ; break;
      case PROTO_MT_REQ_SYNC: cmdstr = "SYN" ; break;
      case PROTO_MT_REQ_HELLO: cmdstr = "HEL" ; break;
      case PROTO_MT_REQ_ACTION: cmdstr = "ACT" ; break;
      case PROTO_MT_REQ_GOODBYE: cmdstr = "BYE" ; break;
      default: cmdstr = "UNK" ; break;
  }

  fprintf(stdout,
    "%s\t"COLOR_YELLOW"%s"COLOR_END
    "\t[%1d]\trc:%s\t%dms\n",
   timestr,cmdstr,action,rcstr,sec);
  fflush(stdout);

}

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
    int ii,new_x,new_y,cur_x,cur_y;
    Player player;
    Player* player_ptr;
    Player_Update_Types update_type;
    for(ii = 0; ii < num_elements; ii++)
    {
        bzero(&player,sizeof(Player));
        if(!decompress_is_ignoreable(&player_compress[ii])) 
        {
            decompress_player(&player,&player_compress[ii],&update_type);

            if(update_type == PLAYER_DROPPED)
            {
               //Get pointer to player from Plist
               player_ptr =  &(maze->players[player.team].at[player.id]);
               //Get current position of the player
               cur_x = player_ptr->client_position.x;
               cur_y = player_ptr->client_position.y;
               
               bzero(player_ptr,sizeof(Player));
               
               
               //Delete player's existance from old cell
               maze->get[cur_x][cur_y].player = NULL;
               if(maze->get[cur_x][cur_y].object==NULL)
                   maze->get[cur_x][cur_y].cell_state = CELLSTATE_EMPTY;
               else
                   maze->get[cur_x][cur_y].cell_state = CELLSTATE_HOLDING;
               
            }
            else
            {
               //Get pointer to player from Plist
               player_ptr =  &(maze->players[player.team].at[player.id]);
              
               //Get current position of the player
               cur_x = player_ptr->client_position.x;
               cur_y = player_ptr->client_position.y;
               
               //Get next position of the player
               new_x = player.client_position.x;
               new_y = player.client_position.y; 

               //Update player's client position to new coordinates
               player_ptr->client_position.x = new_x;
               player_ptr->client_position.y = new_y;

               player_ptr->id = player.id;
               player_ptr->state = player.state;
               player_ptr->team = player.team;

               
               //Delete player's existance from old cell
               maze->get[cur_x][cur_y].player = NULL;
               if(maze->get[cur_x][cur_y].object==NULL)
                   maze->get[cur_x][cur_y].cell_state = CELLSTATE_EMPTY;
               else
                   maze->get[cur_x][cur_y].cell_state = CELLSTATE_HOLDING;

               //Set the player pointer at cell position x,y to my player
               maze->get[new_x][new_y].player = player_ptr; 
               if(maze->get[new_x][new_y].object==NULL)
                   maze->get[new_x][new_y].cell_state = CELLSTATE_OCCUPIED;
               else
                   maze->get[new_x][new_y].cell_state = CELLSTATE_OCCUPIED_HOLDING;

           }
           if(proto_debug())
           {
                fprintf(stderr,"Player Location Update x:%d y:%d\n",player_ptr->client_position.x,player_ptr->client_position.y);
           }
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
    int ii,new_x,new_y,cur_x,cur_y;
    int cur_has_player;
    Object object;
    Object* object_ptr;
    Player* player;
    for(ii = 0; ii < num_elements; ii++)
    {
        bzero(&object,sizeof(Object));
        if(!decompress_is_ignoreable(&object_compress[ii])) 
        {
            decompress_object(&object,&object_compress[ii]);
            
            //Get pointer to object from object list
            object_ptr = object_get(maze,object.type,object.team);
            
            if(num_elements==4)
            {
                cur_x = object_ptr->cell->pos.x;     
                cur_y = object_ptr->cell->pos.y;
            }
            else
            {
                //Get objects current coordinates
                cur_x = object_ptr->client_position.x;
                cur_y = object_ptr->client_position.y;
            }
            //Get new coordinates for the object
            new_x = object.client_position.x;
            new_y = object.client_position.y;

            
            //Get previous state of whether object was held
            cur_has_player = object_ptr->client_has_player;
            

            
            
            //Object Dropped
            if(cur_has_player && !object.client_has_player)
            {
                //Get a pointer to the player that dropped the object
                player = &maze->players[object_ptr->client_player_team].at[object_ptr->client_player_id];

                //Set the player's object pointer to null
                if(object_ptr->type==OBJECT_SHOVEL)
                    player->shovel = NULL;
                else
                    player->flag = NULL;

                //If a player is currently in the same cell as the object then cell state is
                // CELLSTATE_OCCUPIED_HOLDING else CELLSTATE_HOLDING
                if(player->client_position.x == new_x && player->client_position.y == new_y)
                    maze->get[new_x][new_y].cell_state = CELLSTATE_OCCUPIED_HOLDING;
                else
                    maze->get[new_x][new_y].cell_state = CELLSTATE_HOLDING;

                maze->get[new_x][new_y].object = object_ptr;
            }
            //Object Picked up
            else if(!cur_has_player && object.client_has_player)
            {
                //Delete old pointer to the object
                maze->get[cur_x][cur_y].object = NULL;

                //Get the pointer to the player picking up
                player = &maze->players[object.client_player_team].at[object.client_player_id];

                //Set the player's object pointer to the object_ptr obtained from the object list
                if(object.type==OBJECT_SHOVEL)
                    player->shovel = object_ptr;
                else
                    player->flag = object_ptr;

                //Set the cell to an occupied state
                maze->get[new_x][new_y].cell_state = CELLSTATE_OCCUPIED;
                maze->get[new_x][new_y].object = NULL;
            }
            else if(!object.client_has_player)
            {
                if(maze->get[new_x][new_y].player==NULL)
                    maze->get[new_x][new_y].cell_state = CELLSTATE_HOLDING;
                else
                    maze->get[new_x][new_y].cell_state = CELLSTATE_OCCUPIED_HOLDING;
                maze->get[cur_x][cur_y].object = NULL;
                maze->get[new_x][new_y].object = object_ptr;
            }

            //Update object location and other parameters
            object_ptr->client_position.x = new_x;
            object_ptr->client_position.y = new_y;
            object_ptr->team = object.team;
            object_ptr->client_has_player = object.client_has_player;
            object_ptr->client_player_id = object.client_player_id;
            object_ptr->client_player_team = object.client_player_team;
            object_ptr->type = object.type;



            if(proto_debug())
            {
                fprintf(stderr,"Object Update x:%d y:%d\n",object_ptr->client_position.x,object_ptr->client_position.y);
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
            if(proto_debug())
            {
                fprintf(stderr,"Broken Wall Update x:%d y:%d\n",x,y);
            }
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
    request->type = PROTO_MT_REQ_ACTION;
    request->action_type = action;
    request->team = client->my_player->team;
    if(action==ACTION_MOVE)
    {
        if(current)
        {
            request->current.x = current->x;
            request->current.y = current->y;
        }
        if(next)
        {
            request->next.x = next->x;
            request->next.y = next->y;
        }
        
    }
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
int process_hello_request(Maze* maze, Player** my_player, Proto_Msg_Hdr* hdr)
{

   if(hdr->gstate.v0.raw>=0)
   {   
       //Get the team and player id from the header
       Team_Types team = hdr->pstate.v1.raw;
       int id = hdr->pstate.v0.raw;
       
       //Set local Client player pointer to corresponding player in the plist
       *my_player = &(maze->players[team].at[id]);

       (*my_player)->id = id;
       (*my_player)->team = team;

       maze->client_player = *my_player;
    }

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
int process_action_request(Blocking_Helper* bh ,Proto_Msg_Hdr* hdr, Proto_Client_Handle ch)
{
    int result;
    get_int(ch,0,&result);
    bh->RPC_update_id = result;
    return hdr->gstate.v0.raw;
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

    if(proto_debug())
    {
    }

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
            rc = process_hello_request(&C->maze,&C->my_player,&hdr);
            break;
        case PROTO_MT_REQ_GOODBYE:
            rc = process_goodbye_request(&hdr);
            break;
        case PROTO_MT_REP_ACTION:
            rc = process_action_request(&C->bh,&hdr,C->ph);
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



/*  client_init
    Initialize the client data structure
   
    parameter: C            pointer client data structure
    return:    int          1 or -1 for success or failure respectively
*/
int client_init(Client *C,Proto_MT_Handler update_handler)
{
  // Zero global scope
  bzero(C, sizeof(Client));

  // Set connected state to zero
  C->connected = 0; 

  //Set UI size
  C->width = 700;
  C->height = 700;
  
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
  clock_t clk = clock();
  
  // Unpack the request
  Client *C;
  Proto_Msg_Hdr hdr;
  bzero(&hdr,sizeof(Proto_Msg_Hdr));

  C = request->client;

  switch (request->type) {
  case PROTO_MT_REQ_HELLO:  
    if(proto_debug()) fprintf(stderr,"HELLO COMMAND ISSUED");
    hdr.type = request->type;
    rc = do_no_body_rpc(C->ph,&hdr);
    if (proto_debug()) fprintf(stderr,"hello: rc=%x\n", rc);
    if (rc < 0) fprintf(stderr, "Unable to connect");
    break;
  case PROTO_MT_REQ_ACTION:
    if(proto_debug()) fprintf(stderr,"Action COMMAND ISSUED");
    hdr.type = request->type;
    hdr.gstate.v1.raw = request->action_type;
    hdr.pstate.v1.raw = request->team;
    hdr.pstate.v0.raw = C->my_player->id;
    rc = do_action_request_rpc(C->ph,&hdr,request->current,request->next);
    break;
  case PROTO_MT_REQ_SYNC:
    if (proto_debug() ) fprintf(stderr,"Sync COMMAND ISSUED");
    hdr.type = request->type;
    rc = do_no_body_rpc(C->ph,&hdr);
    break;
  case PROTO_MT_REQ_GOODBYE:
    if (proto_debug() ) fprintf(stderr,"Goodbye COMMAND ISSUED");
    hdr.type = request->type;
    rc = do_no_body_rpc(C->ph,&hdr);
    /*rc = proto_client_goodbye(C->ph);*/
    /*printf("Game Over - You Quit");*/
    break;
  default:
    fprintf(stderr,"%s: unknown command %d\n", __func__, request->type);
  }
  
  c_log(request->type,request->action_type,rc,clk);

  // NULL MT OVERRIDE ;-)
  if(proto_debug()) fprintf(stderr,"%s: rc=0x%x\n", __func__, rc);
  if (rc == 0xdeadbeef) rc=1;
  if (proto_debug()) printf("rc=1\n");
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
