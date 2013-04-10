# Bogosort Tests

To run test just use

```shell
cd $BOGOSORT_ROOT
make clean
make 
make test
```

![test example](http://i.imgur.com/6fIAqMM.png)

(Tests only depend on lib/ at the moment, so client and server don't actually have to build)


Running a test works something like this ... 

```c
#include "test.h"

// this is a test function
// it takes a test_context and returns an int
int test_increment(TestContext * tc)
{
    int x,rc;           // setup a return code
    x = 0;              // set x to 
    x++;                // perform increment
    
    // Check that it should equal 1
    should( x==1 , "make zero become one",&tc);
    
    if ( x==1 ) return 0;   // return 0 or more to indicate test passes
    else return -1;         // return -1 to indicate test fails
}

// sample main func
int main(int argc, char** argv)
{
    TestContext tc;             // Allocate space for a test context
    test_init(argc,argv,&tc);   // Initializ the test context
    
    // Run the test with name and context
    run(&test_increment,"Increment",&tc);     
    
    test_summary(&tc);               // Print a summary
}
```

see $BOGOSORT/lib/test.h / 
