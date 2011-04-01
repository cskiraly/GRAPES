ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

OBJS = fifo_queue.o request_handler.o

include $(BASE)/src/utils.mak
