ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

ifeq "$(PLATFORM)" "linux"
	ZAP = zap
else
	ZAP =
endif
ifeq "$(PLATFORM)" "openbsd"
# Can't find major(), minor() in sys/types.h
	DEVRDEV =
else
	DEVRDEV = devrdev
endif

PROGS = filetype access umask changemod unlink $(ZAP) mycd cdpwd $(DEVRDEV)
MOREPROGS = ftw8

all:	$(PROGS) $(MOREPROGS)

%:	%.c $(LIBAPUE)
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LDLIBS)

ftw8:	ftw8.c $(LIBAPUE)
	$(CC) $(CFLAGS) $(NAMEMAX) ftw8.c -o ftw8 $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(MOREPROGS) $(TEMPFILES) *.o $(ZAP) $(DEVRDEV) foo bar times

include $(ROOT)/Make.libapue.inc
