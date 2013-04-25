UNAME:=$(shell uname -s)
ifeq ($(UNAME),Darwin)
	CC:=cc
else
	CC:=gcc
endif

CFLAGS := -g -pthread -Wall
targets=client
src = client.c
objs = $(patsubst %.c,%.o,$(src))
libs = -L../lib -ldagame

all: $(targets)
.PHONY: all

client:  ../lib/libdagame.a
	$(CC) $(CFLAGS) -o client $(src) $(libs)

clean:
	rm -f $(objs) $(targets)
