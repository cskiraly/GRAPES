ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

CFLAGS=-fPIC

OBJS = fifo_queue.o

include $(BASE)/src/utils.mak
