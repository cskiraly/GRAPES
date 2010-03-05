CFLAGS = -g -Wall
CFLAGS += $(call cc-option, -Wdeclaration-after-statement)
CFLAGS += $(call cc-option, -Wno-switch)
CFLAGS += $(call cc-option, -Wdisabled-optimization)
CFLAGS += $(call cc-option, -Wpointer-arith)
CFLAGS += $(call cc-option, -Wredundant-decls)
CFLAGS += $(call cc-option, -Wno-pointer-sign)
CFLAGS += $(call cc-option, -Wcast-qual)
CFLAGS += $(call cc-option, -Wwrite-strings)
CFLAGS += $(call cc-option, -Wtype-limits)
CFLAGS += $(call cc-option, -Wundef)

CFLAGS += $(call cc-option, -funit-at-a-time)

BASE = ../..
TESTS = topology_test \
        topology_test_attr \
        chunk_encoding_test \
        chunk_sending_test \
        chunkidset_test \
        cb_test

CPPFLAGS = -I$(BASE)/include
CPPFLAGS += -I..

LDFLAGS = -L../TopologyManager -L../ChunkTrading -L../ChunkBuffer -L../ChunkIDSet
#LDFLAGS += -static

all: $(TESTS)

topology_test_attr: topology_test_attr.o net_helpers.o
topology_test_attr: ../net_helper.o
topology_test_attr: LDLIBS += -ltopman

topology_test: topology_test.o net_helpers.o
topology_test: ../net_helper.o
topology_test: LDLIBS += -ltopman

chunk_encoding_test: chunk_encoding_test.o
chunk_encoding_test: LDLIBS += -ltrading

cb_test: cb_test.o
cb_test: LDLIBS += -lcb

chunkidset_test: chunkidset_test.o
chunkidset_test: LDLIBS += -lsignalling

chunk_sending_test: chunk_sending_test.o net_helpers.o
chunk_sending_test: ../net_helper.o
chunk_sending_test: LDLIBS += -ltrading

libtopman:
	make -C ../TopologyManager
libtrading:
	make -C ../ChunkTrading

clean::
	rm -f $(TESTS)

allclean: clean
	make -C ../TopologyManager clean
	make -C ../MsgLayer clean

include ../utils.mak
