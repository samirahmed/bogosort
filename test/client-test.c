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

int test_marshall_int_read_buffer(Proto_Session *s, int v)
{
  if (s && ((s->rlen + sizeof(int)) < PROTO_SESSION_BUF_SIZE)) {
    *((int *)(s->rbuf + s->rlen)) = htonl(v);
    s->rlen+=sizeof(int);
    return 1;
  }
  return -1;
}

void test_getcompress_from_rbuf(TestContext* tc)
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


void test_update_map_from_player_compress(TestContext* tc)
{
   int assertion;

   Maze maze;
   maze_build_from_file(&maze,"test.map");

   Player test_player;
   player_init(&test_player);
   test_player.cell = &maze.get[0][0];
   test_player.id = 0;
   test_player.state = PLAYER_FREE;
   test_player.team = TEAM_RED;
    
   int compress; 
   compress_player(&test_player,&compress,PLAYER_ADDED);

   update_players(1,&compress,&maze);

   assertion = (maze.get[0][0].player)==(&maze.players[TEAM_RED].at[0]);
   should("pointer from cell to player matches address of player in the plist",assertion,tc);



}

void st_client_wait_for_event(Task* task)
{
    client_wait_for_event((Blocking_Helper*)(task->arg0));
}

void st_client_signal_update(Task *task)
{
    sleep(3);
    client_signal_update((Blocking_Helper*)(task->arg0));
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
    
    test_task_init(&tasks[0],(Proc)&st_client_wait_for_event,1,&bh,NULL,NULL,NULL,NULL,NULL);
    test_task_init(&tasks[1],(Proc)&st_client_signal_update,1,&bh,NULL,NULL,NULL,NULL,NULL);
    parallelize(tasks,2,thread_per_task);
    
}


int main(int argc, char ** argv )
{
    TestContext tc;
    test_init(argc, argv, &tc);
    
    
    // ADD TESTS HERE
    run(&test_blocking_threads,"Blocking Threads",&tc);
    run(&test_getcompress_from_rbuf,"Get Read Buffer",&tc);
    run(&test_update_map_from_player_compress,"Test map updates",&tc);
    // TEST END HERE

    test_summary(&tc);
    return 0;    
}
