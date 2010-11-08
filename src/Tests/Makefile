ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

TESTS = topology_test \
        topology_test_attr \
        topology_test_th \
        chunk_encoding_test \
        chunk_sending_test \
        chunk_signaling_test \
        chunkidset_test \
        chunkidset_test_bug \
        cb_test \
        config_test \
        tman_test

CPPFLAGS = -I$(BASE)/include

LDFLAGS += -L..
LDLIBS += -lgrapes
#LDFLAGS += -static
ifeq ($(ARCH),win32)
LDLIBS += -lws2_32
endif

all: $(TESTS)

topology_test_attr: topology_test_attr.o net_helpers.o
topology_test_attr: ../net_helper$(NH_INCARNATION).o

topology_test: topology_test.o net_helpers.o
topology_test: ../net_helper$(NH_INCARNATION).o

topology_test_th: topology_test_th.o net_helpers.o
topology_test_th: ../net_helper.o
topology_test_th: CFLAGS += -pthread
topology_test_th: LDFLAGS += -pthread

chunk_encoding_test: chunk_encoding_test.o

cb_test: cb_test.o

chunkidset_test: chunkidset_test.o chunkid_set_h.o

chunkidset_test_bug: chunkidset_test_bug.o chunkid_set_h.o

chunk_sending_test: chunk_sending_test.o net_helpers.o
chunk_sending_test: ../net_helper$(NH_INCARNATION).o

chunk_signaling_test: chunk_signaling_test.o net_helpers.o chunkid_set_h.o
chunk_signaling_test: ../net_helper$(NH_INCARNATION).o

tman_test: tman_test.o topology.o peer.o net_helpers.o
tman_test: ../net_helper$(NH_INCARNATION).o

clean::
	rm -f $(TESTS)

include $(BASE)/src/utils.mak
