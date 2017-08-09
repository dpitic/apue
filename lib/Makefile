#
# Makefile for misc library.
#
ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

LIBMISC = libapue.a
OBJS = error.o pathalloc.o openmax.o setfl.o clrfl.o prexit.o tellwait.o \
		signal.o prmask.o signalintr.o

all:	$(LIBMISC)

$(LIBMISC): $(OBJS)
	$(AR) rv $(LIBMISC) $?
	$(RANLIB) $(LIBMISC)

clean:
	rm -f *.o a.out core temp.* $(LIBMISC)

include $(ROOT)/Make.libapue.inc
