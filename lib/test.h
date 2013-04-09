#define COLOR_HEADER  "\033[95m" 
#define COLOR_OKBLUE  "\033[94m"
#define COLOR_WARNING "\033[93m"
#define COLOR_OKGREEN "\033[92m"
#define COLOR_FAIL    "\033[91m"
#define COLOR_END     "\033[0m"

#define SYMBOL_TICK "\xe2\x9c\x94"
#define SYMBOL_CROSS "\xe2\x9c\x97"

typedef struct {
int pass;
int fail;
int num;
int verbose;
char * current;
} TestContext;

extern void should(int valid, const char* message, TestContext*tc);
extern void test_summary(TestContext *tc);
extern void test_init(int argc, char** argv, TestContext *tc);
extern void run( int (*func)(TestContext*), char * test_name , TestContext *tc);

