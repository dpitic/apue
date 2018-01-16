ROOT=..
EXTRALIBS = -pthread
PLATFORM = $(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

BAR =

ifeq "$(PLATFORM)" "macos"
	TLOCK =
	EXTRALIBS = -pthread
else
	TLOCK = timedlock
endif
ifeq "$(PLATFORM)" "linux"
	BAR = barrier
	EXTRALIBS = -pthread -lrt -lbsd
endif
ifeq "$(PLATFORM)" "freebsd"
	BAR = barrier
	EXTRALIBS = -pthread
endif
ifeq "$(PLATFORM)" "solaris"
	BAR = barrier
	EXTRALIBS = -lpthread -lrt
endif

PROGS = threadid exitstatus badexit2 cleanup

all:	$(PROGS) mutex1.o mutex2.o mutex3.o rwlock.o condvar.o $(TLOCK) $(BAR)

%:	%.c $(LIBAPUE)
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(TEMPFILES) *.o $(TLOCK) $(BAR)

include $(ROOT)/Make.libapue.inc
