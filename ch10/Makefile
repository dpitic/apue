ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

CLD =

ifeq "$(PLATFORM)" "linux"
	CLD = child
endif
ifeq "$(PLATFORM)" "solaris"
	CLD = child
endif

PROGS = sigusr reenter $(CLD)

all:	$(PROGS)

%:	%.c $(LIBAPUE)
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(TEMPFILES) *.o $(CLD)

include $(ROOT)/Make.libapue.inc
