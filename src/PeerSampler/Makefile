ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

OBJS = peersampler.o ncast.o dummy.o cyclon.o cloudcast.o

all: libpsample.a

include $(BASE)/src/utils.mak
