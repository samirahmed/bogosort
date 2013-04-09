CFLAGS := -g -pthread -Wall
targets=bootstrap server-test client-test commons-test
libs = -L../lib -ldagame

src_client= client-test.c
objs_client = $(patsubst %.c,%.o,$(src_client))

src_commons= commons-test.c
objs_commons = $(patsubst %.c,%.o,$(src_commons))

src_server= server-test.c
objs_server = $(patsubst %.c,%.o,$(src_server))

src = $(wildcard *.c)
objs = $(patsubst %.c,%.o,$(src))

src_test = $(wildcard test*.c)
objs_test = $(patsubst %.c,%.o,$(src_test))

all: $(targets)
.PHONY: all

bootstrap: 
	 gcc -c test.c

$(objs) : $(src)
$(objs_server) : $(src_server)
$(objs_client) : $(src_client)
$(objs_commons) : $(src_commons)

clean:
	rm -f $(objs) $(targets)

server-test: ../lib/libdagame.a
	gcc $(CFLAGS) -o server-test $(src_server) $(objs_test) $(libs)

commons-test: ../lib/libdagame.a
	gcc $(CFLAGS) -o commons-test $(src_commons) $(objs_test) $(libs)

client-test: ../lib/libdagame.a
	gcc $(CFLAGS) -o client-test $(src_client) $(objs_test) $(libs)

test:
	./server-test; ./client-test; ./commons-test;
