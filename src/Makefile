ifndef BASE
BASE = ..
endif
CFGDIR ?= .

SUBDIRS = ChunkIDSet ChunkTrading TopologyManager ChunkBuffer PeerSet Scheduler Chunkiser
COMMON_OBJS = config.o

.PHONY: subdirs $(SUBDIRS)

OBJ_LSTS = $(addsuffix /objs.lst, $(SUBDIRS))

all: libgrapes.a

$(OBJ_LSTS):
	$(MAKE) -C $(dir $@) objs.lst FFDIR=$(FFDIR) GTK=$(GTK)

libgrapes.a: $(OBJ_LSTS) $(COMMON_OBJS)
	$(AR) rcs libgrapes.a `cat $(OBJ_LSTS)` $(COMMON_OBJS)
	rm */*.lst

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
