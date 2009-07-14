BASE = ..

CFLAGS = -Wall
CFLAGS += -Wdeclaration-after-statement
CFLAGS += -Wno-switch -Wpointer-arith -Wredundant-decls
CFLAGS += -Wno-pointer-sign 
CFLAGS += -g

CPPFLAGS = -I$(BASE)/include

LDFLAGS = -L../MsgLayer -L../TopologyManager
LDLIBS = -lmsglayer -ltopman
#LDFLAGS += -static

topology_test: topology_test.o net_helpers.o
topology_test: ../MsgLayer/libmsglayer.a
topology_test: ../TopologyManager/libtopman.a

../TopologyManager/libtopman.a:
	make -C ../TopologyManager
../MsgLayer/libmsglayer.a:
	make -C ../MsgLayer 

clean:
	rm -f topology_test
	rm -f *.o
allclean: clean
	make -C ../TopologyManager clean
	make -C ../MsgLayer clean
