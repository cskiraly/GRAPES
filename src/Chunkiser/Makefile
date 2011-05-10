ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

OBJS = input-stream.o           \
       input-stream-dummy.o     \
       output-stream.o          \
       output-stream-dummy.o

ifneq ($(ARCH),win32)
OBJS += \
       input-stream-dumb.o      \
       input-stream-ts.o        \
       input-stream-udp.o       \
       output-stream-raw.o      \
       output-stream-udp.o
endif

ifdef FFDIR
OBJS += input-stream-avf.o input-stream-ipb.o output-stream-avf.o chunkiser_attrib.o
endif

all: libchunkiser.a

include $(BASE)/src/utils.mak
ifdef FFDIR
CPPFLAGS += -I$(FFDIR)
CPPFLAGS += -DAVF
endif
