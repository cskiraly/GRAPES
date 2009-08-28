BASE = ..

CFLAGS = -Wall
CFLAGS += -Wdeclaration-after-statement
CFLAGS += -Wno-switch -Wpointer-arith -Wredundant-decls
CFLAGS += -Wno-pointer-sign 
CFLAGS += -g

CPPFLAGS = -I$(BASE)/include
CPPFLAGS += -I$(BASE)/TopologyManager

LDFLAGS = -L../TopologyManager
LDLIBS = -ltopman
#LDFLAGS += -static

topology_test: topology_test.o net_helpers.o
topology_test: ../TopologyManager/net_helper.o
topology_test: ../TopologyManager/libtopman.a

../TopologyManager/net_helper.o:
../TopologyManager/libtopman.a:
	make -C ../TopologyManager

clean:
	rm -f topology_test
	rm -f *.o

allclean: clean
	make -C ../TopologyManager clean
	make -C ../MsgLayer clean
