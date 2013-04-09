CFLAGS := -g -pthread -Wall
targets=test
src = test.c
objs = $(patsubst %.c,%.o,$(src))
libs = -L../lib -ldagame

all: $(targets)
.PHONY: all

test: $(objs) ../lib/libdagame.a
	gcc $(CFLAGS) -o test $(objs) $(libs)

$(objs) : $(src)

clean:
	rm $(objs) $(targets)
