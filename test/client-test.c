#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <arpa/inet.h>
#include "../lib/types.h"
#include "../lib/protocol.h"
#include "../lib/protocol_client.c"
#include "../lib/net.h"
#include "../lib/game_commons.h"
#include "../lib/game_client.h"
#include "../lib/test.h"

void client_wait_for_event(Blocking_Helper *bh,TestContext* tc)
{
    int assertion;
    assertion = client_maze_lock(bh)==0;
    should("lock the maze from the waiting thread",assertion,tc);
    while (bh->maze->current_game_state !=GAME_STATE_ACTIVE) 
    {
        assertion = client_maze_cond_wait(bh)==0;
        should("wait and recieve signal for condition variable",assertion,tc);
    }
    assertion = client_maze_unlock(bh)==0;
    should("unlock client maze from waiting thread",assertion,tc);

    pthread_exit(NULL);
}

void client_signal_update(Blocking_Helper *bh,TestContext* tc)
{
    int assertion;
    assertion = client_maze_lock(bh)==0;
    should("lock the maze from signaling thread",assertion,tc);
    
    bh->maze->current_game_state = GAME_STATE_ACTIVE;
    assertion = client_maze_signal(bh)==0;
    should("signal the other thread for a change in condition",assertion,tc);
    
    assertion = client_maze_unlock(bh)==0;
    should("unlock client maze from signaling thread",assertion,tc);

    pthread_exit(NULL);
}

int test_marshall_int_read_buffer(Proto_Session *s, int v)
{
  if (s && ((s->rlen + sizeof(int)) < PROTO_SESSION_BUF_SIZE)) {
    *((int *)(s->rbuf + s->rlen)) = htonl(v);
    s->rlen+=sizeof(int);
    return 1;
  }
  return -1;
}

void test_get_compress_from_body(TestContext* tc)
{
    int ii,assertion;
    char buffer[20];
    char output[1000];
    //Init Client and Map
    Client C;
    client_init(&C);
    //Compress random things
    Proto_Session* s;
    Proto_Client *proto_client = C.ph;
    s = &(proto_client->rpc_session);
    for(ii = 0;ii<20;ii++)
    {
        //I have to use this function because no function in lib will write
        //to the receive buffer
        test_marshall_int_read_buffer(s,ii);
    }

    //Get the compress
    int result;
    int offset;
    offset = 0;
    for(ii = 0;ii<20;ii++)
    {
        offset = get_compress_from_body(C.ph,offset,1,&result);
        assertion = ((result)==ii);
        sprintf(buffer,"%d",ii);
        bzero(output,sizeof(output));
        strcat(output,"unmarshall the number: ");
        strcat(output,buffer);
        should(output,assertion,tc);
    } 
}


void test_update_players_from_compress(TestContext* tc)
{
   int assertion;
   int compress; 

   //Create maze
   Maze maze;
   maze_build_from_file(&maze,"test.map");

   //Initialize a dummy player
   Player test_player;
   player_init(&test_player);
   test_player.cell = &maze.get[0][0];
   test_player.id = 0;
   test_player.state = PLAYER_FREE;
   test_player.team = TEAM_RED;
   
   //Compress Player 
   compress_player(&test_player,&compress,PLAYER_ADDED);

   //Update Maze based on compressed player
   update_players(1,&compress,&maze);


   assertion = (maze.get[0][0].player)==(&maze.players[TEAM_RED].at[0]);
   should("match the address of the player stored in the cell to the address of the player in the plist",assertion,tc);

   assertion = (maze.get[0][0].player->state)==(PLAYER_FREE);
   should("match the player's state before the compress",assertion,tc);
}

void test_update_objects_from_compress(TestContext* tc)
{
   int assertion;
   int red_flag_compress,blue_flag_compress,red_shovel_compress,blue_shovel_compress;

   //Create maze
   Maze maze;
   maze_build_from_file(&maze,"test.map");

   //Initialize a dummy object
   Object* red_flag = object_get(&maze,OBJECT_FLAG,TEAM_RED);
   Object* blue_flag = object_get(&maze,OBJECT_FLAG,TEAM_BLUE);
   Object* red_shovel = object_get(&maze,OBJECT_SHOVEL,TEAM_RED);
   Object* blue_shovel = object_get(&maze,OBJECT_SHOVEL,TEAM_BLUE);
  
   //Position of each object somewhere on the map
   red_flag->cell = &maze.get[50][27];
   blue_flag->cell = &maze.get[99][20];
   red_shovel->cell = &maze.get[34][53];
   blue_shovel->cell = &maze.get[79][22];
  
   //Compress Object 
   compress_object(red_flag,&red_flag_compress);
   compress_object(blue_flag,&blue_flag_compress);
   compress_object(red_shovel,&red_shovel_compress);
   compress_object(blue_shovel,&blue_shovel_compress);

   //Update Maze based on compressed object
   update_objects(1,&red_flag_compress,&maze);
   update_objects(1,&blue_flag_compress,&maze);
   update_objects(1,&red_shovel_compress,&maze);
   update_objects(1,&blue_shovel_compress,&maze);
   
   //Check if pointer to object from object list points to same object from the cell it occupies
   assertion = (object_get(&maze,OBJECT_FLAG,TEAM_RED)->client_position.x == 50) &&
               (object_get(&maze,OBJECT_FLAG,TEAM_RED)->client_position.y == 27);
   should("correctly update (50,27): RED FLAG",assertion,tc);
   assertion = (object_get(&maze,OBJECT_FLAG,TEAM_BLUE)->client_position.x == 99) &&
               (object_get(&maze,OBJECT_FLAG,TEAM_BLUE)->client_position.y == 20);
   should("correctly update (99,20): BLUE FLAG",assertion,tc);
   assertion = (object_get(&maze,OBJECT_SHOVEL,TEAM_RED)->client_position.x == 34) &&
               (object_get(&maze,OBJECT_SHOVEL,TEAM_RED)->client_position.y == 53);
   should("correctly update (34,53): RED SHOVEL",assertion,tc);
   assertion = (object_get(&maze,OBJECT_SHOVEL,TEAM_BLUE)->client_position.x == 79) &&
               (object_get(&maze,OBJECT_SHOVEL,TEAM_BLUE)->client_position.y == 22);
   should("correctly update (79,22): BLUE SHOVEL",assertion,tc);

   //Position of each object somewhere on the map
   red_flag->cell = &maze.get[11][22];
   blue_flag->cell = &maze.get[52][61];
   red_shovel->cell = &maze.get[20][16];
   blue_shovel->cell = &maze.get[12][28];
  
   //Compress Object 
   compress_object(red_flag,&red_flag_compress);
   compress_object(blue_flag,&blue_flag_compress);
   compress_object(red_shovel,&red_shovel_compress);
   compress_object(blue_shovel,&blue_shovel_compress);

   //Update Maze based on compressed object
   update_objects(1,&red_flag_compress,&maze);
   update_objects(1,&blue_flag_compress,&maze);
   update_objects(1,&red_shovel_compress,&maze);
   update_objects(1,&blue_shovel_compress,&maze);
   
   //Check if pointer to object from object list points to same object from the cell it occupies
   assertion = (object_get(&maze,OBJECT_FLAG,TEAM_RED)->client_position.x == 11) &&
               (object_get(&maze,OBJECT_FLAG,TEAM_RED)->client_position.y == 22);
   should("correctly update (11,22): RED FLAG",assertion,tc);
   assertion = (object_get(&maze,OBJECT_FLAG,TEAM_BLUE)->client_position.x == 52) &&
               (object_get(&maze,OBJECT_FLAG,TEAM_BLUE)->client_position.y == 61);
   should("correctly update (52,61): BLUE FLAG",assertion,tc);
   assertion = (object_get(&maze,OBJECT_SHOVEL,TEAM_RED)->client_position.x == 20) &&
               (object_get(&maze,OBJECT_SHOVEL,TEAM_RED)->client_position.y == 16);
   should("correctly update (20,16): RED SHOVEL",assertion,tc);
   assertion = (object_get(&maze,OBJECT_SHOVEL,TEAM_BLUE)->client_position.x == 12) &&
               (object_get(&maze,OBJECT_SHOVEL,TEAM_BLUE)->client_position.y == 28);
   should("correctly update (12,28): BLUE SHOVEL",assertion,tc);
}
void st_client_wait_for_event(Task* task)
{
    client_wait_for_event((Blocking_Helper*)(task->arg0),(TestContext*)(task->arg1));
}

void st_client_signal_update(Task *task)
{
    sleep(1);
    client_signal_update((Blocking_Helper*)(task->arg0),(TestContext*)(task->arg1));
}

void test_blocking_threads(TestContext *tc)
{
    Maze maze;
    Blocking_Helper bh;
    // Task Related Values 
    int thread_per_task;
    thread_per_task = 1;
   
    //Init Methods
    maze_build_from_file(&maze,"test.map");
    blocking_helper_init(&bh);
    blocking_helper_set_maze(&bh,&maze);
    
    // Create Tasks
    Task tasks[2];
    bzero(tasks, sizeof(Task)*2 );
    
    test_task_init(&tasks[0],(Proc)&st_client_wait_for_event,1,&bh,tc,NULL,NULL,NULL,NULL);
    test_task_init(&tasks[1],(Proc)&st_client_signal_update,1,&bh,tc,NULL,NULL,NULL,NULL);
    parallelize(tasks,2,thread_per_task);
    
}


int main(int argc, char ** argv )
{
    TestContext tc;
    test_init(argc, argv, &tc);
    
    
    // ADD TESTS HERE
    run(&test_blocking_threads,"Conditional wait and signal",&tc);
    run(&test_get_compress_from_body,"Get read buffer",&tc);
    run(&test_update_players_from_compress,"Update players from compress",&tc);
    run(&test_update_objects_from_compress,"Update objects from compress",&tc);
    // TEST END HERE

    test_summary(&tc);
    return 0;    
}
