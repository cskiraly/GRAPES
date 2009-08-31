BASE = ../..

CFLAGS = -Wall
CFLAGS += -Wdeclaration-after-statement
CFLAGS += -Wno-switch -Wpointer-arith -Wredundant-decls
CFLAGS += -Wno-pointer-sign 
CFLAGS += -g

CPPFLAGS = -I$(BASE)/include
CPPFLAGS += -I../TopologyManager

LDFLAGS = -L../TopologyManager -L../ChunkTrading
LDLIBS = -ltopman -ltrading
#LDFLAGS += -static

all: topology_test chunk_encoding_test

topology_test: topology_test.o net_helpers.o
topology_test: ../TopologyManager/net_helper.o
topology_test: ../TopologyManager/libtopman.a

chunk_encoding_test: chunk_encoding_test.o
chunk_encoding_test: ../ChunkTrading/libtrading.a

../TopologyManager/net_helper.o:
../TopologyManager/libtopman.a:
	make -C ../TopologyManager
../ChunkTrading/libtrading.a:
	make -C ../ChunkTrading

clean:
	rm -f topology_test chunk_encoding_test
	rm -f *.o

allclean: clean
	make -C ../TopologyManager clean
	make -C ../MsgLayer clean
