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

/*Maze maze;  // global maze*/

void test_home(TestContext * tc)
{
    int assertion, key, team;
    Maze maze;
    maze_build_from_file(&maze,"test.map");
    
    Cell * cell;
    assertion = 1;
    for ( team =0; team<NUM_TEAMS ; team++)
    {
        for ( key = -1000 ; key<1000000 && assertion ;key++ )
        { 
          server_hash_id(&maze, key, &cell, team); 
          assertion = ( cell->pos.x < maze.max.x && cell->pos.x >= maze.min.x &&
                        cell->pos.y < maze.max.y && cell->pos.y >= maze.min.y && assertion);
        }
    }
    should("hash any integer to a valid home position",assertion,tc);
    
    key = 16;
    /*server_hash_id(&maze,key,&cell,);

    maze_destroy(&maze);
}

void test_server_locks(TestContext * tc)
{
    Maze maze;
    maze_build_from_file(&maze,"test.map");
    
    if (tc->verbose) fprintf(stderr,"Starting recursive jail lock test\n");
    int ii, times;
    times = 20;
    for (ii=0; ii<times ;ii++)
    {
        server_jail_lock(&maze.jail[TEAM_RED]);
        server_jail_lock(&maze.jail[TEAM_BLUE]);
    }
    if (tc->verbose) fprintf(stderr,"We are %d jail locks deep\n",times);

    for (ii=0; ii<times ;ii++)
    {
        server_jail_unlock(&maze.jail[TEAM_BLUE]);
        server_jail_unlock(&maze.jail[TEAM_RED]);
    }
    if (tc->verbose) fprintf(stderr,"Exited reentrant locks\n");
    should("allow of re-entry and exit of locks",1,tc); 
    
    maze_destroy(&maze);
}

int main(int argc, char ** argv )
{
    TestContext tc;
    test_init(argc, argv, &tc);
    
    // ADD TESTS HERE
    run(&test_server_locks,"Server Locks",&tc);
    run(&test_home,"Home",&tc);
    
    // TEST END HERE
    
    test_summary(&tc);
    return 0;    
}
