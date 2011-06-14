ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

UTILS_DIR = ../Utils

OBJS = cloud_helper.o cloud_helper_utils.o
DELEGATE_HELPERS = libs3_delegate_helper.so mysql_delegate_helper.so
DELEGATE_HELPERS_DEPS = ../Utils/request_handler.o \
			../Utils/fifo_queue.o \
			../config.o \
			../net_helper$(NH_INCARNATION).o

CFLAGS += -I$(UTILS_DIR)

ifdef DELEGATE
OBJS += cloud_helper_delegate.o
CPPFLAGS += -DDELEGATE
endif

all: $(OBJS)

include $(BASE)/src/utils.mak

delegate_helpers: clean-root $(DELEGATE_HELPERS)

libs3_delegate_helper.so: LDFLAGS += -pthread
ifeq ($(PLATFORM), darwin)
libs3_delegate_helper.so: LDFLAGS += -dynamiclib
else
libs3_delegate_helper.so: LDFLAGS += -shared
endif
libs3_delegate_helper.so: LDFLAGS += -ls3
libs3_delegate_helper.so: CFLAGS += -fPIC
libs3_delegate_helper.so: libs3_delegate_helper.o $(DELEGATE_HELPERS_DEPS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

mysql_delegate_helper.so: LDFLAGS += -pthread
ifeq ($(PLATFORM), darwin)
mysql_delegate_helper.so: LDFLAGS += -dynamiclib
else
mysql_delegate_helper.so: LDFLAGS += -shared
endif
mysql_delegate_helper.so: LDFLAGS += -lmysql
mysql_delegate_helper.so: CFLAGS += -fPIC
mysql_delegate_helper.so: mysql_delegate_helper.o $(DELEGATE_HELPERS_DEPS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean-root
clean-root:
	$(MAKE) -C .. clean

clean::
	rm -f *.so
