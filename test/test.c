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

#define COLOR_HEADER  "\033[95m" 
#define COLOR_OKBLUE  "\033[94m"
#define COLOR_WARNING "\033[93m"
#define COLOR_OKGREEN "\033[92m"
#define COLOR_FAIL    "\033[91m"
#define COLOR_END     "\033[0m"

#define SYMBOL_TICK "\xe2\x9c\x94"
#define SYMBOL_CROSS "\xe2\x9c\x97"

Maze maze;  // global maze
int pass;
int fail;
int num;
int verbose;
char * current;

void should( int valid , const char* message)
{
    if (!valid) printf("  "COLOR_FAIL SYMBOL_CROSS COLOR_END " %s should %s\n" , current, message ); 
    if (valid && verbose) printf("  "COLOR_OKGREEN SYMBOL_TICK COLOR_END " %s should %s\n", current, message );
}

int test_maze_load()
{
    int rc;
    rc = maze_build_from_file(&maze,"test.map");
    should(rc>0,"build from file without errors");
    if (rc<0) return rc;

    rc = (maze.min.x == 0 && maze.min.y == 0) && (maze.max.x == 200 && maze.max.y == 200);
    should(rc,"build with correct dimensions");
    if (!rc) return -1;

    rc = (maze.home[TEAM_RED].min.x == 2   && maze.home[TEAM_RED].min.y == 90) &&
         (maze.home[TEAM_RED].max.x == 12  && maze.home[TEAM_RED].max.y == 109) &&
         (maze.home[TEAM_BLUE].min.x == 188 && maze.home[TEAM_BLUE].min.y == 90) &&
         (maze.home[TEAM_BLUE].max.x == 198 && maze.home[TEAM_BLUE].max.y == 109);
    should(rc,"know where the homebase min and max positions are");
    if (!rc) return rc;
   
    rc = (maze.jail[TEAM_RED].min.x == 90   && maze.jail[TEAM_RED].min.y == 90) &&
         (maze.jail[TEAM_RED].max.x == 98  && maze.jail[TEAM_RED].max.y == 109) &&
         (maze.jail[TEAM_BLUE].min.x == 102 && maze.jail[TEAM_BLUE].min.y == 90) &&
         (maze.jail[TEAM_BLUE].max.x == 110 && maze.jail[TEAM_BLUE].max.y == 109);
    should(rc,"know where the jail min and max positions are");
    if (!rc) return rc;

    rc = (maze.players[TEAM_RED].count == 0 && maze.players[TEAM_BLUE].count == 0) &&
         (maze.players[TEAM_RED].max < 192 && maze.players[TEAM_BLUE].max < 192);
    should(rc,"know successfully initialize the plists");
    if (!rc) return rc;

    return 1;
}


void run( int (*func)(), char * test_name )
{
    current = test_name;
    printf(COLOR_OKBLUE "%d. %s" COLOR_END "\n" , num++ , test_name ); 
    int result;
    result = (*func)();

    if (result < 0)
    {
      printf(COLOR_FAIL "%s failed \n" COLOR_END "\n" , test_name ); 
      fail++;
    }
    else
    {
      printf( COLOR_OKGREEN "%s passed \n" COLOR_END "\n" , test_name ); 
      pass++;
    }
}

void test_summary()
{
    printf( COLOR_OKBLUE "%d" COLOR_END " test run\n", pass+fail);
    if (pass > 0) printf( COLOR_OKGREEN "%d" COLOR_END " passed, ", pass ); 
    if (fail > 0) printf( COLOR_FAIL "%d" COLOR_END " failed.", fail ); 
    else printf( "no failed tests\n");
}

void test_init(int argc, char** argv)
{
  if (argc > 1)
  {
     if (strncmp(argv[1],"-v",sizeof("-v")-1) == 0) verbose = 1 ;
  }
  
  pass=0;
  fail=0;
  num =0;
}

int main(int argc, char ** argv )
{
    test_init(argc, argv);
      
    // ADD TESTS HERE
    run(&test_maze_load,"Maze Load"); 
    
    // TEST END HERE
    
    test_summary();
    
}
