ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

ifeq "$(PLATFORM)" "linux"
	ZAP = zap
else
	ZAP =
endif

PROGS = filetype access umask changemod unlink $(ZAP) mycd cdpwd devrdev
MOREPROGS = ftw8

all:	$(PROGS) $(MOREPROGS)

%:	%.c $(LIBAPUE)
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(MOREPROGS) $(TEMPFILES) *.o $(ZAP) foo bar times

include $(ROOT)/Make.libapue.inc
