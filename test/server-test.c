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
#include "../lib/game_server.h"
#include "../lib/test.h"

Maze maze;  // global maze

int test_home_counter(TestContext * tc)
{
    maze_build_from_file(&maze,"test.map");
   
    maze_destroy(&maze);
    return 1;
}

int test_server_locks(TestContext * tc)
{
    maze_build_from_file(&maze,"test.map");
    
    printf("Starting recursive jail lock test\n");
    int ii, times;
    times = 20;
    for (ii=0; ii<times ;ii++)
    {
        server_jail_lock(&maze.jail[TEAM_RED]);
        server_jail_lock(&maze.jail[TEAM_BLUE]);
    }
    printf("We are %d jail locks deep\n",times);

    for (ii=0; ii<times ;ii++)
    {
        server_jail_unlock(&maze.jail[TEAM_BLUE]);
        server_jail_unlock(&maze.jail[TEAM_RED]);
    }
    printf("Exited reentrant locks\n");
    should(1,"allow of re-entry and exit of locks",tc); 
    
    maze_destroy(&maze);
    return 1;
}

int main(int argc, char ** argv )
{
    TestContext tc;
    test_init(argc, argv, &tc);
      
    // ADD TESTS HERE
    run(&test_server_locks,"Server Locks",&tc);
    run(&test_home_counter,"Home Counter",&tc);
    
    // TEST END HERE
    
    test_summary(&tc);
    return 0;    
}
