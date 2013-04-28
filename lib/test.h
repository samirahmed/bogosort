#ifndef __DAGAME_UNIT_TEST_H__
#define __DAGAME_UNIT_TEST_H__

#include <signal.h>
#include "types.h"
#define COLOR_HEADER  "\033[95m" 
#define COLOR_OKBLUE  "\033[94m"
#define COLOR_WARNING "\033[93m"
#define COLOR_OKGREEN "\033[92m"
#define COLOR_FAIL    "\033[91m"
#define COLOR_END     "\033[0m"

#define SYMBOL_TICK "\xe2\x9c\x94"
#define SYMBOL_CROSS "\xe2\x9c\x97"

#define BREAKPOINT() raise(SIGUSR1);    // should be caught in gdb - is ignorable (unix only)
#define SUICIDE() raise(SIGUSR1);

typedef void(*Proc)(void*);

typedef struct {
  void * arg0;
  void * arg1;
  void * arg2;
  void * arg3;
  void * arg4;
  void * arg5;
  void (*func)(void*);
  int   reps;
} Task;

typedef void(*TaskFunction)(Task*);

typedef struct {
    int pass;
    int fail;
    int num;
    int verbose;
    char * current;
    int current_test_status;
    int blocking;
    void (*test_function)(void *);
    pthread_mutex_t lock;
    time_t start_time;
    time_t stop_time;
    int single_test;
} TestContext;

typedef void(*TestFunction)(TestContext*);

extern void test_nanosleep(void);
extern void should(const char* message, int valid, TestContext *tc);
extern void test_summary(TestContext *tc);
extern void test_init(int argc, char** argv, TestContext *tc);
extern void run( void (*func)(TestContext*), char * test_name , TestContext *tc);
extern void parallelize(Task task[], int num_tasks, int num_threads );
extern int  test_debug(void);
extern int  randint(void);
extern void test_task_init(Task *task, Proc func, int reps, void* a0,void*a1, void*a2, void*a3,void*a4,void*a5);

#endif
