#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include "test.h"

extern void should( int valid , const char* message, TestContext *tc)
{
    if (!valid) 
      printf("  "COLOR_FAIL SYMBOL_CROSS COLOR_END " %s should %s\n" , tc->current, message ); 
    if (valid && tc->verbose) 
      printf("  "COLOR_OKGREEN SYMBOL_TICK COLOR_END " %s should %s\n", tc->current, message );
}

extern void run( int (*func)(TestContext*), char * test_name , TestContext *tc)
{
    tc->current = test_name;
    printf(COLOR_OKBLUE "%d. %s" COLOR_END "\n" , tc->num++ , test_name ); 
    int result;
    result = (*func)(tc);

    if (result < 0)
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
