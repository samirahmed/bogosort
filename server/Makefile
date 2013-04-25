UNAME:=$(shell uname -s)
ifeq ($(UNAME),Darwin)
	CC:=cc
else
	CC:=gcc
endif

CFLAGS := -g -pthread -Wall
targets=server
src = server.c
objs = $(patsubst %.c,%.o,$(src))
libs = -L../lib -ldagame

all: $(targets)
.PHONY: all

server: $(objs) ../lib/libdagame.a
	$(CC) $(CFLAGS) -o server $(objs) $(libs)

$(objs) : $(src)

clean:
	rm -f $(objs) $(targets)
