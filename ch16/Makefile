ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

ifeq "$(PLATFORM)" "solaris"
	EXTRALIBS = -lsocket -lnsl
endif

PROGS = ruptime
MOREPROGS = findsvc

all:	$(PROGS) $(MOREPROGS) clconn.o clconn2.o initsrv1.o

%:	%.c $(LIBAPUE)

ruptime:	ruptime.o clconn2.o $(LIBAPUE)
	$(CC) $(CFLAGS) -o ruptime ruptime.o clconn2.o $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(MOREPROGS) $(TEMPFILES) *.o

include $(ROOT)/Make.libapue.inc

