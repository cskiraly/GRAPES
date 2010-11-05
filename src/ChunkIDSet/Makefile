ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

OBJS = chunkids_ops.o chunkids_ha.o chunkids_encoding.o

all: libsignalling.a

include $(BASE)/src/utils.mak
