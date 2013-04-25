#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "types.h"
#include "test.h"
#include "protocol_utils.h"

// Create a task from a bunch of arguments
extern void test_task_init(Task *task, Proc func, int reps, void* a0,void*a1, void*a2, void*a3,void*a4,void*a5)
{
  task->func = func;
  task->reps = reps;
  task->arg0 = a0;
  task->arg1 = a1;
  task->arg2 = a2;
  task->arg3 = a3;
  task->arg4 = a4;
  task->arg5 = a5;
}

// just performs a nanosleep and returns
void test_nanosleep(void)
{
  struct timespec desired, diff;
  bzero(&desired,sizeof(struct timespec));
  desired.tv_sec = 0;
  desired.tv_nsec = (randint() % 5);
  nanosleep(&desired,&diff);
}

// this is the function that is run in a thread
// we repeat the task n times and then exit 
void * threaded_task(void* task_ptr)
{
  int reps,ii;
  struct timespec desired, diff;
  bzero(&desired,sizeof(struct timespec));
  
  // extract task* and reps (min resp = 1) 
  Task* task = (Task*) task_ptr;
  reps = task->reps < 1 ?  1 : task->reps;
  
  for (ii = 0; ii<reps; ii++)
  {
    desired.tv_sec = 0;
    desired.tv_nsec = (randint() % 10);
    
    TaskFunction function = (TaskFunction) task->func;
    (*function)(task);

    nanosleep(&desired,&diff);
  }
  
  pthread_exit( (void*) 0); 
}

// Runs an array of "num_task" tasks called "tasks" accross "num_threads"
// Simulates concurrent threads
// Create task with the test_task_init method
// These are probablistic test
extern void parallelize(Task tasks[], int num_tasks, int threads_per_task)
{
  int ii, thread_count;

  // create joinable attr
  pthread_attr_t attr; 
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
  pthread_attr_setstacksize(&attr,PTHREAD_STACK_SIZE);
  struct sched_param param;
  bzero(&param,sizeof(struct sched_param));

  // create new array of n threads
  thread_count = (threads_per_task < 1 ? 1 : threads_per_task*num_tasks);
  pthread_t * threads;
  threads = (pthread_t *) malloc(sizeof(pthread_t)*thread_count);

  int min = sched_get_priority_min(SCHED_OTHER); 
  int max = sched_get_priority_max(SCHED_OTHER);
  for (ii=0;ii < thread_count ;ii++)
  {
#ifdef __APPLE__
    // Randomized Priority
    param.sched_priority = (randint()%max)+min;
    pthread_attr_setschedparam(&attr, &param);
#endif
    // Spawn thread
    int rc;
    rc = pthread_create(&threads[ii], &attr, threaded_task, (void*) &tasks[ii%num_tasks] );
    if (test_debug() && rc!=0 ) 
      fprintf(stderr,"%s_%d pthread_create failed %d\n",__FILE__,__LINE__,rc);
  }
  
  if (proto_debug() ) fprintf(stderr,"%s_%d main thread suspending\n", __FILE__, __LINE__ );
 
  for (ii=0;ii < thread_count ;ii++)
  {
    pthread_join(threads[ii],NULL);  // Honeybadger the exit status
  }
  
  if (proto_debug() ) fprintf(stderr,"%s_%d main thread resuming\n", __FILE__, __LINE__ );

  // clean up
  free(threads);
  pthread_attr_destroy(&attr);
}

// Should is syntatic sugar for an assertion.  
// Should stores test results in the test context
// Should pretty prints assertion results with ticks and cross
// Should is not thread safe.
extern void should(const char* message, int valid, TestContext *tc)
{
    if (!valid) 
    {
      fprintf(stderr,"  "COLOR_FAIL SYMBOL_CROSS COLOR_END " %s should %s\n" , tc->current, message ); 
      //BREAKPOINT();
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

    tc->num++;
    if (tc->single_test > 0 && tc->single_test != tc->num ) return;
    tc->current = test_name;
    tc->test_function = (void(*)(void*)) func;
    
    fprintf(stderr, COLOR_OKBLUE "%d. %s" COLOR_END "\n" , tc->num , test_name ); 
    
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

/************/
/* HELPERS  */
/************/

// debug alias
extern int test_debug()
{
  return proto_debug();
}

/*****************/
/* TEST CONTEXT  */
/*****************/

// print a pretty sumamry from the given test context object
// includes the total failed / passed tests
extern void test_summary(TestContext *tc)
{
    // calc diff
    int seconds;
    tc->stop_time = time(NULL);
    seconds = tc->stop_time - tc->start_time; 

    // report time
    fprintf(stderr, COLOR_OKBLUE "%d" COLOR_END " test run", tc->pass+tc->fail);
    if (seconds>0) fprintf(stderr, " in %d seconds", seconds);
    fprintf(stderr,"\n");
    
    // print report
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
      if (strncmp(argv[argument],"--only",sizeof("--only")-1) == 0)
      {
        if (argument+1<argc)
        {
          int only;
          if ( (only = atoi(argv[argument+1]) ) >0 )
          {
            tc->single_test = only;
          }
        }
      }
    }
  }
  
  tc->pass=0;
  tc->fail=0;
  tc->num =0;
  tc->start_time = time(NULL);  
  fprintf(stderr,"\n" COLOR_HEADER "%s---------------------\n" COLOR_END "\n",argv[0]);
}
