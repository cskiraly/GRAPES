ifndef BASE
BASE = ..
endif
CFGDIR ?= .

SUBDIRS = ChunkIDSet ChunkTrading TopologyManager ChunkBuffer PeerSet Scheduler Cache PeerSampler CloudSupport Utils
ifneq ($(ARCH),win32)
  SUBDIRS += Chunkiser
endif
COMMON_OBJS = config.o

OBJ_LSTS = $(addsuffix /objs.lst, $(SUBDIRS))

NET_HELPERS = $(addsuffix .o, $(basename $(wildcard net_helper-*.c)))

.PHONY: $(OBJ_LSTS)

all: libgrapes.a

$(OBJ_LSTS):
	$(MAKE) -C $(dir $@) objs.lst FFDIR=$(FFDIR) GTK=$(GTK)

libgrapes.a: $(OBJ_LSTS) $(COMMON_OBJS)
	$(AR) rcs libgrapes.a `cat $(OBJ_LSTS)` $(COMMON_OBJS)

NH_TCP_SRCS := $(wildcard NetHelper/*c)
net_helper-tcp.o : net_helper-tcp.c $(NH_TCP_SRCS:.c=.o)
	$(CC) $(CFLAGS) -I../include -I. -INetHelper/ -c $< -o $@.part
	ld -r $(NH_TCP_SRCS:.c=.o) $@.part -o $@

tests: libgrapes.a
	$(MAKE) -C Tests

clean::
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

allclean: clean
	rm -f *.o *.a *.o.part
	rm -f *.d
	$(MAKE) -C Tests clean

include $(BASE)/src/utils.mak
