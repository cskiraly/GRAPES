BASE = ..
SUBDIRS = ChunkIDSet ChunkTrading TopologyManager ChunkBuffer PeerSet Scheduler
COMMON_OBJS = config.o

.PHONY: subdirs $(SUBDIRS)

#COMMON_O = $(addsuffix /$(COMMON), $(SUBDIRS))
OBJ_LSTS = ChunkIDSet/objs.lst ChunkTrading/objs.lst TopologyManager/objs.lst ChunkBuffer/objs.lst PeerSet/objs.lst Scheduler/objs.lst

all: libgrapes.a

$(OBJ_LSTS):
	$(MAKE) -C $(dir $@) objs.lst

libgrapes.a: $(OBJ_LSTS) $(COMMON_OBJS)
	ar rcs libgrapes.a `cat $(OBJ_LSTS)` $(COMMON_OBJS)

tests: libgrapes.a
	$(MAKE) -C Tests

clean::
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

allclean: clean
	rm -f *.o *.a
	$(MAKE) -C Tests clean

include utils.mak
