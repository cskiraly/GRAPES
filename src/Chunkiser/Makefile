ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

OBJS = input-stream.o           \
       input-stream-dummy.o     \
       input-stream-dumb.o      \
       input-stream-ts.o        \
       input-stream-udp.o       \
       output-stream.o          \
       output-stream-raw.o      \
       output-stream-udp.o      \
       output-stream-dummy.o 

ifdef FFDIR
OBJS += input-stream-avf.o output-stream-avf.o
endif

all: libchunkiser.a

include $(BASE)/src/utils.mak
ifdef FFDIR
CPPFLAGS += -I$(FFDIR)
CPPFLAGS += -DAVF
endif
