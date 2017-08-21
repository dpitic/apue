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

PROGS = sigusr reenter $(CLD) read1 read2 critical mask suspend1 suspend2
MOREPROGS = tsleep2

all:	$(PROGS) $(MOREPROGS) sleep1.o sleep2.o abort.o

%:	%.c $(LIBAPUE)
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LDLIBS)

tsleep2:	tsleep2.o sleep2.o $(LIBAPUE)
	$(CC) $(CFLAGS) -o tsleep2 tsleep2.o sleep2.o $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(MOREPROGS) $(TEMPFILES) *.o $(CLD)

include $(ROOT)/Make.libapue.inc
