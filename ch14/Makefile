ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

ifeq "$(PLATFORM)" "linux"
	EXTRALIBS = -lrt
endif
ifeq "$(PLATFORM)" "solaris"
	EXTRALIBS = -lrt
endif

PROGS = nonblockw deadlock
MOREPROGS =

all:	$(PROGS) $(MOREPROGS)

%:	%.c $(LIBAPUE)
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(MOREPROGS) $(TEMPFILES) *.o xlate rot13c2.c

include $(ROOT)/Make.libapue.inc
