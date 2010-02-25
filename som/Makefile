SUBDIRS = ChunkIDSet ChunkTrading TopologyManager ChunkBuffer PeerSet Scheduler
     
.PHONY: subdirs $(SUBDIRS)
     
libgrapes.a:
	rm -f $@
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir libcommon; \
	done	

tests: libgrapes.a
	$(MAKE) -C Tests

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

allclean: clean
	rm -f *.o *.a
	$(MAKE) -C Tests clean
