CFLAGS := -g
targets=client
src = client.c
objs = $(patsubst %.c,%.o,$(src))
libs = -L../lib -ldagame

all: $(targets)
.PHONY: all

client: $(objs) ../lib/libdagame.a
	gcc $(CFLAGS) -o client $(objs) $(libs)

$(objs) : $(src)

clean:
	rm $(objs) $(targets)
