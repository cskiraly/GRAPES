ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

UTILS_DIR = ../Utils

OBJS = cloud_helper.o cloud_helper_utils.o cloud_helper_delegate.o
DELEGATE_HELPERS = libs3_delegate_helper.so mysql_delegate_helper.so

CFLAGS += -I$(UTILS_DIR)

all: $(OBJS)

include $(BASE)/src/utils.mak

delegate_helpers: $(DELEGATE_HELPERS)


libs3_delegate_helper.so: CFLAGS += -fPIC
libs3_delegate_helper.so: ../Utils/request_handler.o ../Utils/fifo_queue.o ../config.o ../net_helper$(NH_INCARNATION).o
libs3_delegate_helper.so: libs3_delegate_helper.o
libs3_delegate_helper.so: CFLAGS += -shared -pthread
ifeq (${PLATFORM}, darwin)
libs3_delegate_helper.so: CFLAGS += -dynamiclib
endif
libs3_delegate_helper.so: LDFLAGS += -ls3
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)


mysql_delegate_helper.so: CFLAGS += -fPIC
mysql_delegate_helper.so: ../Utils/request_handler.o ../Utils/fifo_queue.o ../config.o ../net_helper$(NH_INCARNATION).o
mysql_delegate_helper.so: mysql_delegate_helper.o
mysql_delegate_helper.so: CFLAGS += -shared -pthread
ifeq (${PLATFORM}, darwin)
mysql_delegate_helper.so: CFLAGS += -dynamiclib
endif
mysql_delegate_helper.so: LDFLAGS += -lmysqlclient
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean::
	rm -f *.so
