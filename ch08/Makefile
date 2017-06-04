ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

PROGS = fork1 vfork1 wait1 fork2 tellwait1 tellwait2 exec1 echoall exec2 printuids
MOREPROGS = systest1 systest3

all:	$(PROGS) $(MOREPROGS) system.o

%:	%.c $(LIBAPUE)
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LDLIBS)

systest1:	system.o systest1.o $(LIBAPUE)
	$(CC) $(CFLAGS) -o systest1 systest1.o system.o $(LDFLAGS) $(LDLIBS)

systest3:	system.o systest3.o $(LIBAPUE)
	$(CC) $(CFLAGS) -o systest3 systest3.o system.o $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(MOREPROGS) $(TEMPFILES) *.o

include $(ROOT)/Make.libapue.inc
