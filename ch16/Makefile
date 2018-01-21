ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

ifeq "$(PLATFORM)" "solaris"
	EXTRALIBS = -lsocket -lnsl
endif

PROGS =
MOREPROGS = findsvc

all:	$(PROGS) $(MOREPROGS) clconn.o clconn2.o initsrv1.o

%:	%.c $(LIBAPUE)

clean:
	rm -f $(PROGS) $(MOREPROGS) $(TEMPFILES) *.o

include $(ROOT)/Make.libapue.inc

