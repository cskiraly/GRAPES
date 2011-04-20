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

.PHONY: $(OBJ_LSTS)

all: libgrapes.a

$(OBJ_LSTS):
	$(MAKE) -C $(dir $@) objs.lst FFDIR=$(FFDIR)

libgrapes.a: $(OBJ_LSTS) $(COMMON_OBJS)
	$(AR) rcs libgrapes.a `cat $(OBJ_LSTS)` $(COMMON_OBJS)

tests: libgrapes.a
	$(MAKE) -C Tests

clean::
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

allclean: clean
	rm -f *.o *.a
	rm -f *.d
	$(MAKE) -C Tests clean

include $(BASE)/src/utils.mak
