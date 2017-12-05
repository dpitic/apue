ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

all:	init.o

clean:
	rm -f $(TEMPFILES) *.o
