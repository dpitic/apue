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

PROGS = sigusr reenter read1 read2 critical mask suspend1 suspend2 sigstp
MOREPROGS = tsleep2 systest2

all:	$(PROGS) $(MOREPROGS) sleep1.o sleep2.o abort.o system.o $(CLD)

%:	%.c $(LIBAPUE)
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LDLIBS)

tsleep2:	tsleep2.o sleep2.o $(LIBAPUE)
	$(CC) $(CFLAGS) -o tsleep2 tsleep2.o sleep2.o $(LDFLAGS) $(LDLIBS)

systest2:	systest2.o system.o $(LIBAPUE)
	$(CC) $(CFLAGS) -o systest2 systest2.o system.o $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(MOREPROGS) $(TEMPFILES) *.o $(CLD) temp.foo

include $(ROOT)/Make.libapue.inc
