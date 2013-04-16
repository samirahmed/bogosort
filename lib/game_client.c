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
<<<<<<< HEAD
#include "game_client.h"

extern void blocking_helper_init(Blocking_Helper *bh)
{
    pthread_mutex_init(&bh->maze_lock,NULL);
    pthread_cond_init(&bh->maze_updated,NULL);
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
>>>>>>> Added funtion headers to blocking logic
}
