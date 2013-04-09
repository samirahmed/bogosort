UNAME:=$(shell uname -s)
ifeq ($(UNAME),Darwin)
	CC:=cc
else
	CC:=gcc
endif

CFLAGS := -g -pthread -Wall
targets= server-test client-test commons-test
libs = -L../lib -ldagame

src_client= client-test.c
objs_client = $(patsubst %.c,%.o,$(src_client))

src_commons= commons-test.c
objs_commons = $(patsubst %.c,%.o,$(src_commons))

src_server= server-test.c
objs_server = $(patsubst %.c,%.o,$(src_server))

src = $(wildcard *.c)
objs = $(patsubst %.c,%.o,$(src))

all: $(targets)
.PHONY: all

$(objs) : $(src)
$(objs_server) : $(src_server)
$(objs_client) : $(src_client)
$(objs_commons) : $(src_commons)

server-test: ../lib/libdagame.a
	$(CC) $(CFLAGS) -o server-test $(src_server)  $(libs)

commons-test: ../lib/libdagame.a
	$(CC) $(CFLAGS) -o commons-test $(src_commons)  $(libs)

client-test: ../lib/libdagame.a
	$(CC) $(CFLAGS) -o client-test $(src_client) $(libs)

clean:
	rm -f $(objs) $(targets)

test:
	./server-test -v; ./client-test -v; ./commons-test -v;
