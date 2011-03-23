ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

OBJS = cloud_helper.o cloud_helper_utils.o cloud_helper_delegate.o

include $(BASE)/src/utils.mak
