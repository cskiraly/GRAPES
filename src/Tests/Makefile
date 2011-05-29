ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

NH_INCARNATION ?= udp
NET_HELPER = ../net_helper-$(NH_INCARNATION)

TESTS = topology_test \
        topology_test_attr \
        chunk_encoding_test \
        chunk_sending_test \
        chunk_signaling_test \
        chunkidset_test \
        chunkidset_test_bug \
        cb_test \
        config_test \
        tman_test \
        topo_msg_size_test \
	cloud_test \
	cloudcast_topology_test \
	cloud_topology_monitor \
	test_queue

ifneq ($(ARCH),win32)
  TESTS += topology_test_th \
           chunkiser_test
endif

CPPFLAGS = -I$(BASE)/include

LDFLAGS += -L..
LDLIBS += -lgrapes
#LDFLAGS += -static

ifdef DELEGATE
LDLIBS += -ldl
endif

all: $(TESTS)

include $(BASE)/src/utils.mak

ifeq ($(ARCH),win32)
LDLIBS += -lws2_32
endif

topo_msg_size_test: $(NET_HELPER).o

topology_test_attr: topology_test_attr.o net_helpers.o
topology_test_attr: $(NET_HELPER).o

topology_test: topology_test.o net_helpers.o
topology_test: $(NET_HELPER).o

topology_test_th: topology_test_th.o net_helpers.o
topology_test_th: $(NET_HELPER).o
topology_test_th: CFLAGS += -pthread
topology_test_th: LDFLAGS += -pthread

chunk_encoding_test: chunk_encoding_test.o

cb_test: cb_test.o

chunkidset_test: chunkidset_test.o chunkid_set_h.o

chunkidset_test_bug: chunkidset_test_bug.o chunkid_set_h.o

chunk_sending_test: chunk_sending_test.o net_helpers.o
chunk_sending_test: $(NET_HELPER).o

chunk_signaling_test: chunk_signaling_test.o net_helpers.o chunkid_set_h.o
chunk_signaling_test: $(NET_HELPER).o

tman_test: tman_test.o topology.o peer.o net_helpers.o
tman_test: $(NET_HELPER).o

chunkiser_test: chunkiser_test.o
chunkiser_test: $(NET_HELPER).o
ifdef FFDIR
chunkiser_test: CFLAGS += -pthread
chunkiser_test: LDFLAGS += -pthread -L$(FFDIR)/libavutil -L$(FFDIR)/libavcore -L$(FFDIR)/libavcodec -L$(FFDIR)/libavformat
chunkiser_test: LDLIBS += -lm -lavformat -lavcodec -lavutil
chunkiser_test: LDLIBS += $(call ld-option, -lavcore)
chunkiser_test: LDLIBS += $(call ld-option, -lz)
chunkiser_test: LDLIBS += $(call ld-option, -lbz2)
endif

cloud_test: cloud_test.o net_helpers.o
cloud_test: $(NET_HELPER).o
cloud_test: LDFLAGS += -ldl

cloudcast_topology_test: cloudcast_topology_test.o net_helpers.o
cloudcast_topology_test: $(NET_HELPER).o
cloudcast_topology_test: CFLAGS += -pthread
cloudcast_topology_test: LDFLAGS += -ldl -pthread

cloud_topology_monitor: cloud_topology_monitor.o net_helpers.o
cloud_topology_monitor: $(NET_HELPER).o
cloud_topology_monitor: LDFLAGS += -ldl

test_queue: test_queue.o
test_queue: CFLAGS += -I$(BASE)/src/Utils

clean::
	rm -f $(TESTS)
	rm -f $(DEPENDENCIES)
