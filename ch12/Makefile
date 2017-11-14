ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

TOUT =
ifeq "$(PLATFORM)" "freebsd"
	EXTRALIBS = -pthread
endif
ifeq "$(PLATFOMR)" "linux"
	EXTRALIBS = -pthread
	TOUT =
endif
ifeq "$(PLATFORM)" "solaris"
	EXTRALIBS = -lpthread
	TOUT =
endif

PROGS =

all:	$(PROGS) detach.o $(TOUT)

%:	%.c $(LIBAPUE)
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(TEMPFILES) *.o

include $(ROOT)/Make.libapue.inc