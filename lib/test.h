#include <signal.h>

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
} TestContext;

typedef void(*TestFunction)(TestContext*);

extern void should(const char* message, int valid, TestContext *tc);
extern void test_summary(TestContext *tc);
extern void test_init(int argc, char** argv, TestContext *tc);
extern void run( void (*func)(TestContext*), char * test_name , TestContext *tc);
extern void run_two_thread( void* (*func_one)(void*), void* (*func_two)(void*),char *test_name, TestContext *tc);


