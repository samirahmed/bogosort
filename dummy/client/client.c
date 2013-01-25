#include <stdio.h>
#include "../lib/dummy.h"
int main(int argc, char **argv){
	printf("server main: HELLO\n");
	dummy_hello();
	printf("server main: GOODBYE\n");
	return 0;
}
