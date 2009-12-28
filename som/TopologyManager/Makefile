BASE = ../..

CFLAGS = -Wall
CFLAGS += -Wdeclaration-after-statement
CFLAGS += -Wno-switch -Wpointer-arith -Wredundant-decls
CFLAGS += -Wno-pointer-sign 
CFLAGS += -g

CPPFLAGS = -I$(BASE)/include -I..

OBJS = ncast.o ncast_proto.o nccache.o

all: libtopman.a

%.a: $(OBJS)
	ar rcs $@ $^

clean:
	rm -f *.a
	rm -f *.o
