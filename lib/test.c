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

extern void where(void)
{
  fprintf(stderr,"In %s at %d\n", __FILE__, __LINE__ );
}

extern void should( int valid , const char* message, TestContext *tc)
{
    if (!valid) 
    {
      printf("  "COLOR_FAIL SYMBOL_CROSS COLOR_END " %s should %s\n" , tc->current, message ); 
      where();
      BREAKPOINT();
      tc->current_test_status=-1;
      pthread_exit( (void*)-1 );
    }
    if (valid && tc->verbose) 
      printf("  "COLOR_OKGREEN SYMBOL_TICK COLOR_END " %s should %s\n", tc->current, message );

}

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

extern void run( void (*func)(TestContext*), char * test_name , TestContext *tc)
{
    int rc;
    void* status;
    pthread_t thread;
    pthread_attr_t attr;

    tc->current = test_name;
    tc->test_function = (void(*)(void*)) func;

    printf(COLOR_OKBLUE "%d. %s" COLOR_END "\n" , tc->num++ , test_name ); 
    
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
    pthread_attr_setstacksize(&attr,PTHREAD_STACK_SIZE*1000 );
    pthread_create(&thread, &attr, threaded_test, (void*) tc );
    pthread_attr_destroy(&attr);
   
    BREAKPOINT();
    rc = pthread_join( thread, &status); 
    if (rc == EINVAL || rc == EDEADLK )
    {
      printf(COLOR_FAIL "Test Exception in %s line %d - pthread_join returned %d \n" COLOR_END , 
        __FILE__, __LINE__,rc );
    }
    else
    {
        if (tc->current_test_status < 0)
        {
          printf(COLOR_FAIL "%s failed \n" COLOR_END "\n" , test_name ); 
          tc->fail++;
        }
        else
        {
          printf( COLOR_OKGREEN "%s passed \n" COLOR_END "\n" , test_name ); 
          tc->pass++;
        }
    }
}

extern void test_summary(TestContext *tc)
{
    printf( COLOR_OKBLUE "%d" COLOR_END " test run\n", tc->num);
    if (tc->pass > 0) printf( COLOR_OKGREEN "%d" COLOR_END " passed, ", tc->pass ); 
    if (tc->fail > 0) printf( COLOR_FAIL "%d" COLOR_END " failed.", tc->fail ); 
    else printf( "no failed tests\n");
}

extern void test_init(int argc, char** argv, TestContext *tc)
{
  bzero(tc,sizeof(TestContext));
  if (argc > 1)
  {
     if (strncmp(argv[1],"-v",sizeof("-v")-1) == 0) tc->verbose = 1 ;
  }
  
  tc->pass=0;
  tc->fail=0;
  tc->num =0;
  
  printf("\n" COLOR_HEADER "%s---------------------\n" COLOR_END "\n",argv[0]);
}
