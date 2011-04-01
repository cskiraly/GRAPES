ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

UTILS_DIR = ../Utils

OBJS = cloud_helper.o cloud_helper_utils.o cloud_helper_delegate.o
DELEGATE_HELPERS = libs3_delegate_helper.so

CFLAGS += -I$(UTILS_DIR)

all: $(OBJS)

include $(BASE)/src/utils.mak

delegate_helpers: $(DELEGATE_HELPERS)

libs3_delegate_helper.so: ../net_helper$(NH_INCARNATION).o $(UTILS_DIR)/request_handler.o $(UTILS_DIR)/fifo_queue.o ../config.o
libs3_delegate_helper.so: CFLAGS += -shared -pthread
libs3_delegate_helper.so: LDFLAGS += -ls3
libs3_delegate_helper.so: libs3_delegate_helper.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

libs3_delegate_helper.o: CFLAGS += -fPIC

clean::
	rm -f *.so