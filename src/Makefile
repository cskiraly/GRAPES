BASE = ..
SUBDIRS = ChunkIDSet ChunkTrading TopologyManager ChunkBuffer PeerSet Scheduler
COMMON_OBJS = config.o

.PHONY: subdirs $(SUBDIRS)

#COMMON_O = $(addsuffix /$(COMMON), $(SUBDIRS))
COMMON_O = ChunkIDSet/common.o ChunkTrading/common.o TopologyManager/common.o ChunkBuffer/common.o PeerSet/common.o Scheduler/common.o

all: libgrapes.a

$(COMMON_O):
	$(MAKE) -C $(dir $@) $(COMMON)

libgrapes.a: $(COMMON_O) $(COMMON_OBJS)

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
