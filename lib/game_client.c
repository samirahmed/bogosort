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

extern void blocking_helper_init(Blocking_Helper *bh)
{
    bzero(bh,sizeof(Blocking_Helper));
    pthread_mutex_init(&bh->maze_lock,NULL);
    pthread_cond_init(&bh->maze_updated,NULL);
}

extern void blocking_helper_set_maze(Blocking_Helper *bh, Maze* m)
{
    bh->maze = m;    
}

extern void blocking_helper_destroy(Blocking_Helper *bh)
{
    pthread_mutex_destroy(&bh->maze_lock);
    pthread_cond_destroy(&bh->maze_updated);    
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
