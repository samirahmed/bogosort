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
targets = libdagame.a
src = $(wildcard *.c)
objs = $(patsubst %.c,%.o,$(src))

all: $(targets)
.PHONY : all

libdagame.a: $(objs)
	ar -c -r libdagame.a $(objs)

$(objs) : $(src)

clean:
	rm -f $(objs) $(targets)
