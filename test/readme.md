# Bogosort Tests

## Arguments

| command | description |
|---------|-------------|
|`-d` or `--debug`| enable `proto_debug` spew (might not work on linux)|
|`-v` or `--verbose`| enable verbose mode - prints all assertion info |
|`--only <test number>`| run only the specified test (1 indexed), i.e `--only 5` for the fifth test|

## Running Tests

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

## Writing your own

```c
#include "test.h"

// this is a test function
// it takes a test_context that asserts using "should" clauses
void test_increment(TestContext * tc)
{
    int x,rc;           // setup a return code
    x = 0;              // set x to 
    x++;                // perform increment
    
    // Check that it should equal 1
    should("make zero become one",x==1,&tc); 
    // should will automatic trigger a break point on failure
    // if the assertion fails, the function will stop executing
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

## Test Framework

The `TestContext` object is a variable passed through your tests to share information 
between tests themselves and the test framework code.

Each test runs on its own thread, one test at a time. When a test finishes its thread 
joins the main thread and execution continues

the framework provides the following functions

- **Assertions**  `should("do someting",assertion,tc)` 
- **Run Tests**  `run(&testHome,"Home",&testcontext);` 

## Parallelized Tasks

To write parallelized tasks see the `lib/test.c` file.

Create a 'task' function which matchs the `void task(void*)` prototype.

Then pass this to the `parallelize()` function, which will run the task 
as many times are you specify on as many threads per task as you specify.

for example of parallelized tasks see the `test_server_locks` function in `test/server-test.c`
## Debugging

A `SIGINFO` is raised whenever an assertion fails. If you just run the test in gdb

`gdb commons-test` and run the executable. You will automatically be stopped at the `SIGINFO` from where you use the 'next' and 'frame' command to debug the process;

see `$BOGOSORT/lib/test.c` for implementation details 
