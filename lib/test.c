#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "types.h"
#include "test.h"
#include "protocol_utils.h"

// Should is syntatic sugar for an assertion.  
// Should stores test results in the test context
// Should pretty prints assertion results with ticks and cross
// Should is not thread safe.
extern void should(const char* message, int valid, TestContext *tc)
{
    if (!valid) 
    {
      fprintf(stderr,"  "COLOR_FAIL SYMBOL_CROSS COLOR_END " %s should %s\n" , tc->current, message ); 
      BREAKPOINT();
      tc->current_test_status=-1;
      pthread_exit( (void*)-1 );
    }
    if (valid && tc->verbose) 
    {
      fprintf(stderr,"  "COLOR_OKGREEN SYMBOL_TICK COLOR_END " %s should %s\n", tc->current, message );
    }
}

// Threaded tests take as input a pointer to a TestContext
// The function then casts some c magic to exec the associated test function, passing itself as an argument
// All paths lead to pthread_exit. current_status = 1 if successful
// Only one threaded_test should be run per context
void* threaded_test( void* arg)
{
  TestContext* tc = (TestContext*) arg;
  TestFunction test_function;
  test_function = (TestFunction) tc->test_function;
  tc->current_test_status = 0; 
  (*test_function)(tc);
  tc->current_test_status = 1;  //redundant but for testing purposes
  pthread_exit( (void*) 0); 
}

// Run takes as input a test, a test name and a test context and executes the test in a new thread
// Main thread execution suspends until the test thread completes
extern void run( void (*func)(TestContext*), char * test_name , TestContext *tc)
{
    int rc;
    void* status;
    pthread_t thread;
    pthread_attr_t attr;

    tc->current = test_name;
    tc->test_function = (void(*)(void*)) func;

    fprintf(stderr, COLOR_OKBLUE "%d. %s" COLOR_END "\n" , tc->num++ , test_name ); 
    
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
    pthread_attr_setstacksize(&attr,PTHREAD_STACK_SIZE*100 );    //~1.6mb max stack size per thread 1.6*400 < 500mb RAM
    pthread_create(&thread, &attr, threaded_test, (void*) tc );
    pthread_attr_destroy(&attr);
   
    rc = pthread_join( thread, &status); 
    if (rc == EINVAL || rc == EDEADLK )
    {
      fprintf(stderr,COLOR_FAIL "Test Exception in %s line %d - pthread_join returned %d \n" COLOR_END , 
        __FILE__, __LINE__,rc );
    }
    else
    {
        if (tc->current_test_status < 0)
        {
          fprintf(stderr,COLOR_FAIL "%s failed \n" COLOR_END "\n" , test_name ); 
          tc->fail++;
        }
        else
        {
          fprintf(stderr, COLOR_OKGREEN "%s passed \n" COLOR_END "\n" , test_name ); 
          tc->pass++;
        }
    }
}

// print a pretty sumamry from the given test context object
// includes the total failed / passed tests
extern void test_summary(TestContext *tc)
{
    fprintf(stderr, COLOR_OKBLUE "%d" COLOR_END " test run\n", tc->num);
    if (tc->pass > 0) fprintf(stderr, COLOR_OKGREEN "%d" COLOR_END " passed, ", tc->pass ); 
    if (tc->fail > 0) fprintf(stderr, COLOR_FAIL "%d" COLOR_END " failed.", tc->fail ); 
    else fprintf(stderr, "no failed tests\n");
}

// test_init is used to configure TestContext variables given a set of command line arguments
//  -v, --verbose : enable verbose test output (show all assertions etc)
//  -d, --debug   : enable debug spew to stderr (potentially tonnes of crap)
extern void test_init(int argc, char** argv, TestContext *tc)
{
  bzero(tc,sizeof(TestContext));
  if (argc > 1)
  {
    int argument;
    for (argument = 1; argument<argc; argument++)
    {
      if (strncmp(argv[argument],"-v",sizeof("-v")-1) == 0) tc->verbose = 1 ;
      if (strncmp(argv[argument],"--verbose",sizeof("--verbose")-1) == 0) tc->verbose = 1 ;
      if (strncmp(argv[argument],"-d",sizeof("-d")-1) == 0) { proto_debug_on() ; tc->verbose=1; }
      if (strncmp(argv[argument],"--debug",sizeof("--debug")-1) == 0){ proto_debug_on() ; tc->verbose=1;}
    }
  }
  
  tc->pass=0;
  tc->fail=0;
  tc->num =0;
  
  fprintf(stderr,"\n" COLOR_HEADER "%s---------------------\n" COLOR_END "\n",argv[0]);
}
