SUBDIRS = ChunkIDSet ChunkTrading TopologyManager ChunkBuffer
     
.PHONY: subdirs $(SUBDIRS)
     
all:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done

tests: all
	$(MAKE) -C Tests

libsom.a:
	rm -f $@
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir libcommon; \
	done	

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

allclean: clean
	rm -f *.o
	$(MAKE) -C Tests clean
