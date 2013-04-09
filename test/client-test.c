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
#include "../lib/test.h"

int main(int argc, char ** argv )
{
    TestContext tc;
    test_init(argc, argv, &tc);
      
    // ADD TESTS BELOW (like the example)
    // run(&test_client_object,"Client Object",&tc); 
    
    // TEST END HERE
    
    test_summary(&tc);
    return 0;    
}
