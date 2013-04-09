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
#include "test.h"

Maze maze;  // global maze

int main(int argc, char ** argv )
{
    TestContext tc;
    test_init(argc, argv, &tc);
      
    // ADD TESTS HERE
    /*run(&test_maze_load,"Maze Load",&tc); */

    
    // TEST END HERE
    
    test_summary(&tc);
    return 0;    
}
