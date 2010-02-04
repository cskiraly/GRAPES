BASE = ../..

CFLAGS = -Wall
CFLAGS += -Wdeclaration-after-statement
CFLAGS += -Wno-switch -Wpointer-arith -Wredundant-decls
CFLAGS += -Wno-pointer-sign 
CFLAGS += -g

CPPFLAGS = -I$(BASE)/include

OBJS = sched.o sched_random.o

all: libsched.a

%.a: $(OBJS)
	ar rcs $@ $^

clean:
	rm -f *.a
	rm -f *.o
