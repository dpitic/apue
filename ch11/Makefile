ROOT=..
EXTRALIBS = -pthread
PLATFORM = $(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

BAR =

ifeq "$(PLATFORM)" "macos"
	TLOCK =
	EXTRALIBS = -pthread
else
	TLOCK = 
endif
ifeq "$(PLATFORM)" "linux"
	BAR = 
	EXTRALIBS = -pthread -lrt -lbsd
endif
ifeq "$(PLATFORM)" "freebsd"
	BAR = barrier
	EXTRALIBS = -pthread
endif
ifeq "$(PLATFORM)" "solaris"
	BAR = barrier
	EXTRALIBS = -lpthread -ltr
endif

PROGS = threadid exitstatus badexit2 cleanup

all:	$(PROGS) $(TLOCK) $(BAR)

%:	%.c $(LIBAPUE)
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(TEMPFILES) *.o $(TLOCK) $(BAR)

include $(ROOT)/Make.libapue.inc
