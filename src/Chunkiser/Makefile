ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

OBJS = input-stream.o           \
       input-stream-dummy.o     \
       input-stream-dumb.o      \
       input-stream-udp.o       \
       output-stream.o          \
       output-stream-raw.o      \
       output-stream-udp.o      \
       output-stream-dummy.o 

ifdef FFDIR
OBJS += input-stream-avf.o output-stream-avf.o
ifdef GTK
OBJS += output-stream-play.o
endif
endif

all: libchunkiser.a

include $(BASE)/src/utils.mak
ifdef FFDIR
output-stream-avf.o: CPPFLAGS += -I$(FFDIR)
input-stream-avf.o: CPPFLAGS += -I$(FFDIR)
input-stream-ipb.o: CPPFLAGS += -I$(FFDIR)
output-stream-play.o: CPPFLAGS += -I$(FFDIR)
CPPFLAGS += -DAVF
ifdef GTK
output-stream-play.o: CPPFLAGS += `pkg-config --cflags gtk+-2.0`
CPPFLAGS += -DGTK
endif
endif
