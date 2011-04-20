ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

OBJS = ncast_proto.o cyclon_proto.o topo_proto.o topocache.o blist_cache.o blist_proto.o cloudcast_proto.o

all: libnodecache.a

include $(BASE)/src/utils.mak
