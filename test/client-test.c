#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include "../lib/types.h"
#include "../lib/protocol.h"
#include "../lib/net.h"
#include "../lib/game_commons.h"
#include "../lib/game_client.h"
#include "../lib/test.h"

// Global Variables shared by threads
Blocking_Helper bh;
Maze map;

void* signal_function(void *tc) 
{
    printf("Thread signal_function lock\n");
    client_maze_lock(&bh);
    printf("Thread signal_function change game state\n");
    bh.maze->current_game_state = GAME_STATE_ACTIVE;
    printf("Thread signal_function signal to other thread that state has changed\n");
    client_maze_signal(&bh);
    printf("Thread signal_function unlock\n");
    client_maze_unlock(&bh);
    pthread_exit(NULL);
}

void* wait_function(void *tc) 
{
    printf("Thread wait_function lock\n");
    client_maze_lock(&bh);
    printf("Thread wait_function waiting for condition variable\n");
    while (bh.maze->current_game_state !=GAME_STATE_ACTIVE) 
        client_maze_cond_wait(&bh);
    printf("Thread wait_function unlocks\n");
    client_maze_unlock(&bh);
    pthread_exit(NULL);
}




int main(int argc, char ** argv )
{
    TestContext tc;
    test_init(argc, argv, &tc);
    
    //Init Blocking Data Structure
    blocking_helper_init(&bh);
    bh.maze = &map;
    
    // ADD TESTS HERE
    run_two_thread(&signal_function,&wait_function,"Blocking Thread",&tc);
    // TEST END HERE

    //Destroy Blocking Data Structure
    blocking_helper_destroy(&bh);
    test_summary(&tc);
    return 0;    
}
