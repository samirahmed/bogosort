# Copyright (C) 2011 by Jonathan Appavoo, Boston University

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

UISYSTEM=$(shell uname)

ifeq ($(UISYSTEM),Darwin)
  UIINCDIR = -I/opt/local/include
  UILIBS = -L/opt/local/lib -lSDLmain -lSDL -lSDL_image -framework Cocoa
else
  UINCDIR = 
  UILIBS = -lSDL
endif

CFLAGS := -g $(UIINCDIR) -std=c99

MODULE := $(shell basename $CURDIR)

DAGAMELIBHDRS := types.h net.h protocol.h protocol_utils.h            \
	protocol_session.h protocol_client.h protocol_server.h maze.h \
	player.h ui.h game.h
DAGAMELIBFILE := libdagame.a
DAGAMELIBARCHIVE := ../lib/$(DAGAMELIBFILE)
DAGAMELIB := -L../lib -ldagame


src  = $(wildcard *.c)
objs = $(patsubst %.c,%.o,$(src))

ifeq ($(MODULE),lib)
  DAGAMELIBINCS:=$(DAGAMELIBHDRS)
else
  DAGAMELIBINCS:=$(addprefix ../lib/,$(DAGAMELIBHDRS))
endif


all: $(targets)
.PHONY: all

$(objs) : $(src) $(DAGAMELIBINCS)

clean:
	rm $(objs) $(targets)

