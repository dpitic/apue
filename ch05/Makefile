ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

ifeq "$(PLATFORM)" "linux"
	MEMSTR = MEMSTR
else
	MEMSTR =
endif

PROGS =

all:	$(PROGS) $(MEMSTR)

%:	%.c $(LIBAPUE)
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(TEMPFILES) *.o $(MEMSTR)

include $(ROOT)/Make.libapue.inc
