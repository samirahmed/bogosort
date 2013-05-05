UNAME:=$(shell uname -s)
UISYSTEM=$(shell uname)

ifeq ($(UNAME),Darwin)
	CC:=cc
else
	CC:=gcc
endif

ifeq ($(UISYSTEM),Darwin)
  UIINCDIR = -I/opt/local/include
  UILIBS = -L/opt/local/lib -lSDLmain -lSDL -lSDL_image -lX11 -framework Cocoa
else
  UINCDIR = 
  UILIBS = -lSDL -lX11
endif

CFLAGS := -g $(UIINCDIR) -pthread -Wall
targets=server
src = server.c
objs = $(patsubst %.c,%.o,$(src))
libs = -L../lib -ldagame

all: $(targets)
.PHONY: all

server: $(objs) ../lib/libdagame.a
	$(CC) $(CFLAGS) -o server $(src) $(libs) $(UILIBS)
#$(CC) $(CFLAGS) -o server $(objs) $(libs)

$(objs) : $(src)

clean:
	rm -f $(objs) $(targets)
