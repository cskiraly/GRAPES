ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

OBJS = chunk_encoding.o chunk_delivery.o chunk_signaling.o

all: libtrading.a

include $(BASE)/src/utils.mak
