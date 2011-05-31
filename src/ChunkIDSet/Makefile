ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

OBJS = chunkids_ops.o chunkids_ha.o chunkids_encoding.o chunkids_ops_list.o chunkids_ops_set.o chunkids_encoding_list.o chunkids_encoding_set.o

all: libsignalling.a

include $(BASE)/src/utils.mak
