cc-option = $(shell if $(CC) $(1) -S -o /dev/null -xc /dev/null \
              > /dev/null 2>&1; then echo "$(1)"; fi ;)

ld-option = $(shell if echo "int main(){return 0;}" | \
		$(CC) $(CFLAGS) $(1) -o /dev/null -xc - \
		> /dev/null 2>&1; then echo "$(1)"; fi ;)



CFLAGS += -g -Wall
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

ifdef GPROF
CFLAGS += -pg
LDFLAGS += -pg
endif

CPPFLAGS = -I$(BASE)/include -I$(BASE)/src

LIBCOMMON = libgrapes.a
COMMON = common.o

%.a: $(OBJS)
	ar rcs $@ $^

libcommon: $(OBJS)
	ar rcs $(BASE)/src/$(LIBCOMMON) $^

objs.lst: $(OBJS)
	echo $(addprefix `pwd`/, $(OBJS)) > objs.lst

$(COMMON): objs.lst
	ld -r -o $(COMMON) `cat objs.lst`
clean::
	rm -f *.a
	rm -f *.o
	rm -f *.lst
	rm -f *.d

### Automatic generation of headers dependencies ###
%.d: %.c
	$(CC) $(CPPFLAGS) -MM -MF $@ $<

%.o: %.d

-include $(OBJS:.o=.d)
