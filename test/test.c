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
#define COLOR_WANRING "\033[93m"
#define COLOR_OKGREEN "\033[92m"
#define COLOR_FAIL    "\033[91m"
#define COLOR_END     "\033[0m"

Maze maze;  // global maze
int pass;
int fail;
int num;

int test_maze_load()
{
    return maze_build_from_file(&maze,"test.map");
}

void run( int (*func)(), const char * test_name )
{
    printf(COLOR_OKBLUE "%d. %s" COLOR_END "\n" , num++ , test_name ); 
    int result;
    result = (*func)();

    if (result < 0)
    {
      printf(COLOR_FAIL "%s failed \n" COLOR_END "---\n" , test_name ); 
      fail++;
    }
    else
    {
      printf( COLOR_OKGREEN "%s passed \n" COLOR_END "---\n" , test_name ); 
      pass++;
    }
}

void test_summary()
{
    printf( COLOR_OKBLUE "TOTAL RUN : %d\n" COLOR_END, pass+fail);
    if (pass > 0)printf( COLOR_OKGREEN "TOTAL PASSED : %d\n" COLOR_END, pass ); 
    if (fail > 0)printf( COLOR_FAIL "TOTAL FAILED : %d\n" COLOR_END, fail ); 
}

void test_init()
{
  pass=0;
  fail=0;
  num =0;
}

int main(int argc, char ** argv )
{
    test_init();
      
    // ADD TESTS HERE
    run(&test_maze_load,"Maze Load Test"); 
    
    // TEST END HERE
    
    test_summary();
    
}
