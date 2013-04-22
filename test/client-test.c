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
    // TEST END HERE

    test_summary(&tc);
    return 0;    
}
