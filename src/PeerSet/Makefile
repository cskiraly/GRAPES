ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

OBJS = peerset_ops.o

all: libpeerset.a

include $(BASE)/src/utils.mak
