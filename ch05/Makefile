ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

ifeq "$(PLATFORM)" "linux"
	MEMSTR = memstr
	MEMSTR =
else
	MEMSTR =
endif

PROGS = getcputc fgetsfputs buf tempfiles mkstemp

all:	$(PROGS) $(MEMSTR)

%:	%.c $(LIBAPUE)
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(TEMPFILES) *.o $(MEMSTR) std.out std.err

include $(ROOT)/Make.libapue.inc
