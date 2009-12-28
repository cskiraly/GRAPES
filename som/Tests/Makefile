BASE = ../..
TESTS = topology_test chunk_encoding_test chunk_sending_test

CFLAGS = -Wall
CFLAGS += -Wdeclaration-after-statement
CFLAGS += -Wno-switch -Wpointer-arith -Wredundant-decls
CFLAGS += -Wno-pointer-sign 
CFLAGS += -g

CPPFLAGS = -I$(BASE)/include
CPPFLAGS += -I../TopologyManager
CPPFLAGS += -I..

LDFLAGS = -L../TopologyManager -L../ChunkTrading
#LDFLAGS += -static

all: $(TESTS)

topology_test: topology_test.o net_helpers.o
topology_test: ../net_helper.o
topology_test: LDLIBS += -ltopman

chunk_encoding_test: chunk_encoding_test.o
chunk_encoding_test: ../ChunkTrading/libtrading.a
chunk_encoding_test: LDLIBS += -ltrading

chunk_sending_test: chunk_sending_test.o net_helpers.o
chunk_sending_test: ../net_helper.o
chunk_sending_test: LDLIBS += -ltrading

libtopman:
	make -C ../TopologyManager
libtrading:
	make -C ../ChunkTrading

clean:
	rm -f $(TESTS)
	rm -f *.o

allclean: clean
	make -C ../TopologyManager clean
	make -C ../MsgLayer clean
