UNAME:=$(shell uname -s)
UISYSTEM=$(shell uname)

ifeq ($(UNAME),Darwin)
	CC:=cc
else
	CC:=gcc
endif

ifeq ($(UISYSTEM),Darwin)
  UIINCDIR = -I/opt/local/include
  UILIBS = -L/opt/local/lib -lSDLmain -lSDL -lSDL_image -framework Cocoa
else
  UINCDIR = 
  UILIBS = -lSDL
endif

CFLAGS := -g $(UIINCDIR) -pthread -Wall
targets=client
src = client.c 
objs = $(patsubst %.c,%.o,$(src))
libs = -L../lib -ldagame 

all: $(targets)
.PHONY: all

client:  ../lib/libdagame.a
	$(CC) $(CFLAGS) -o client $(src) $(libs) $(UILIBS)

clean:
	rm -f $(objs) $(targets)
