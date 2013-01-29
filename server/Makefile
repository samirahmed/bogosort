CFLAGS := -g -pthread
targets=server
src = server.c
objs = $(patsubst %.c,%.o,$(src))
libs = -L../lib -ldagame

all: $(targets)
.PHONY: all

server: $(objs) ../lib/libdagame.a
	gcc $(CFLAGS) -o server $(objs) $(libs)

$(objs) : $(src)

clean:
	rm $(objs) $(targets)
