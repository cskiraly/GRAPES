ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

ifdef FFDIR
OBJS += input-stream-avf.o output-stream-avf.o
endif

all: libchunkiser.a

include $(BASE)/src/utils.mak
CPPFLAGS += -I$(FFDIR)
