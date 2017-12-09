ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

all:	init.o single.o

clean:
	rm -f $(TEMPFILES) *.o
