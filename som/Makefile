SUBDIRS = ChunkIDSet ChunkTrading TopologyManager
     
.PHONY: subdirs $(SUBDIRS)
     
all:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done

tests: all
	make -C Tests

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

allclean: clean
	rm -f *.o
	make -C Tests clean
