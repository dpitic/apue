ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

ifeq "$(PLATFORM)" "freebsd"
  EXTRALIBS=-pthread
endif
ifeq "$(PLATFORM)" "openbsd"
  EXTRALIBS=-pthread
endif
ifeq "$(PLATFORM)" "linux"
  EXTRALIBS=-pthread
endif
ifeq "$(PLATFORM)" "solaris"
  EXTRALIBS=-lsocket -lnsl
endif

PROGS =	pollmsg sendmsg bindunix
FDPASS =

# FreeBSD, OpenBSD and Linux provide support for sending credentials over UNIX
# domain sockets.  macOS credential passing is disabled, and Solaris 10 doesn't
# support sending credentials over UNIX domain sockets; although it supports the
# ability to obtain the credentials of a process passing a file descriptor over
# STREAMS pipe.
ifeq "$(PLATFORM)" "linux"
	FDPASS = recvfd2.o sendfd2.o
endif
ifeq "$(PLATFORM)" "freebsd"
	FDPASS = recvfd2.o sendfd2.o
endif
ifeq "$(PLATFORM)" "openbsd"
	FDPASS = recvfd2.o sendfd2.o
#	FDPASS =
endif

all:	$(PROGS) $(FDPASS)
	for i in open open.fe opend.fe; do \
		(cd $$i && $(MAKE) ) || exit 1; \
	done

%:	%.c $(LIBAPUE)
	$(CC) $(CFLAGS) $@.c -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(TEMPFILES) *.o *.socket
	for i in open open.fe opend.fe; do \
		(cd $$i && $(MAKE) clean) || exit 1; \
	done

include $(ROOT)/Make.libapue.inc
