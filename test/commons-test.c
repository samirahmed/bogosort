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

int test_cell(TestContext *tc)
{
    int rc;
    rc = 1;
    maze_build_from_file(&maze, "test.map");
    int xx,yy;
    
    for(xx = maze.min.x ; xx < maze.max.x ; xx++ )
    {
      for(yy = maze.min.y ; yy < maze.max.y ; yy++ )
      {
        cell_lock(&(maze.get[xx][yy])); 
        rc = ((pthread_mutex_trylock(&(maze.get[xx][yy].lock)) != 0) && rc );
        cell_unlock(&(maze.get[xx][yy]));
      }
    }
    rc = rc!=0;
    should(rc!=0,"lock properly",tc);

    if(rc) return 0;
    else return -1;
}

int test_maze_load(TestContext *tc)
{
    int rc;
    rc = maze_build_from_file(&maze,"test.map");
    should(rc>0,"build from file without errors",tc);
    if (rc<0) return rc;

    rc = (maze.min.x == 0 && maze.min.y == 0) && (maze.max.x == 200 && maze.max.y == 200);
    should(rc,"build with correct dimensions",tc);
    if (!rc) return -1;

    rc = (maze.home[TEAM_RED].min.x == 2   && maze.home[TEAM_RED].min.y == 90) &&
         (maze.home[TEAM_RED].max.x == 12  && maze.home[TEAM_RED].max.y == 109) &&
         (maze.home[TEAM_BLUE].min.x == 188 && maze.home[TEAM_BLUE].min.y == 90) &&
         (maze.home[TEAM_BLUE].max.x == 198 && maze.home[TEAM_BLUE].max.y == 109);
    should(rc,"know where the homebase min and max positions are",tc);
    if (!rc) return rc;
   
    rc = (maze.jail[TEAM_RED].min.x == 90   && maze.jail[TEAM_RED].min.y == 90) &&
         (maze.jail[TEAM_RED].max.x == 98  && maze.jail[TEAM_RED].max.y == 109) &&
         (maze.jail[TEAM_BLUE].min.x == 102 && maze.jail[TEAM_BLUE].min.y == 90) &&
         (maze.jail[TEAM_BLUE].max.x == 110 && maze.jail[TEAM_BLUE].max.y == 109);
    should(rc,"know where the jail min and max positions are",tc);
    if (!rc) return rc;

    rc = (maze.players[TEAM_RED].count == 0 && maze.players[TEAM_BLUE].count == 0) &&
         (maze.players[TEAM_RED].max < 192 && maze.players[TEAM_BLUE].max < 192);
    should(rc,"know successfully initialize the plists",tc);
    if (!rc) return rc;

    maze_destroy(&maze);

    return 1;
}

int main(int argc, char ** argv )
{
    TestContext tc;
    test_init(argc, argv, &tc);
      
    // ADD TESTS HERE
    run(&test_maze_load,"Maze Load",&tc); 
    run(&test_cell,"Cells",&tc); 
    
    // TEST END HERE
    
    test_summary(&tc);
    return 0;    
}
