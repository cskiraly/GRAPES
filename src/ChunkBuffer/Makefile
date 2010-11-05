ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

OBJS = buffer.o buffer-ha.o

all: libcb.a

include $(BASE)/src/utils.mak
