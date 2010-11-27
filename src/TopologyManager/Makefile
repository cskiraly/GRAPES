ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

OBJS = peersampler.o topman.o ncast.o ncast_proto.o dummy.o cyclon.o cyclon_proto.o topo_proto.o topocache.o blist_cache.o blist_proto.o tman.o dumbTopman.o cloud_helper.o cloud_helper_delegate.o

all: libtopman.a

include $(BASE)/src/utils.mak
