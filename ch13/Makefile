ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

all:	init.o single.o reread.o reread2.o cloexec.o

clean:
	rm -f $(TEMPFILES) *.o
