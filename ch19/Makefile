ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

ifeq "$(PLATFORM)" "solaris"
    EXTRALIBS=-lsocket -lnsl
endif

PROGS = pty

all:	$(PROGS)

pty:	main.o $(LIBAPUE)
	$(CC) $(CFLAGS) -o pty main.o $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(TEMPFILES) *.o

include $(ROOT)/Make.libapue.inc
