ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

LIBMISC	= libapue_db.a
COMM_OBJ = db.o

ifeq "$(PLATFORM)" "solaris"
# Original link command for Solaris uses first ld found on PATH
#  LDCMD=$(LD) -64 -G -Bdynamic -R/lib/64:/usr/ucblib/sparcv9 -o libapue_db.so.1 -L/lib/64 -L/usr/ucblib/sparcv9 -L$(ROOT)/lib -lapue db.o
# With OpenCSW software installed, must explicitly use full path to Solaris ld,
# otherwise the csw/gnu/ld linker will be invoked in the above command.
  LDCMD=/usr/ccs/bin/ld -64 -G -Bdynamic -R/lib/64:/usr/ucblib/sparcv9 \
  -o libapue_db.so.1 -L/lib/64 -L/usr/ucblib/sparcv9 -L$(ROOT)/lib -lapue db.o
  EXTRALD=-m64 -R.
else
  LDCMD=$(CC) -shared -Wl,-dylib -o libapue_db.so.1 -L$(ROOT)/lib -lapue -lc \
  db.o
endif
ifeq "$(PLATFORM)" "linux"
  EXTRALD=-Wl,-rpath=.
endif
ifeq "$(PLATFORM)" "freebsd"
  EXTRALD=-R.
endif
ifeq "$(PLATFORM)" "openbsd"
  EXTRALD=-R.
endif
ifeq "$(PLATFORM)" "macos"
  EXTRALD=-R.
endif

all: libapue_db.so.1 t4 t4dump $(LIBMISC)

libapue_db.a:	$(COMM_OBJ) $(LIBAPUE)
		$(AR) rsv $(LIBMISC) $(COMM_OBJ)
		$(RANLIB) $(LIBMISC)

libapue_db.so.1:	db.c $(LIBAPUE)
		$(CC) -fPIC $(CFLAGS) -c db.c
		$(LDCMD)
		ln -s libapue_db.so.1 libapue_db.so

t4:	$(LIBAPUE)
		$(CC) $(CFLAGS) -c -I. t4.c
		$(CC) $(EXTRALD) -o t4 t4.o -L$(ROOT)/lib -L. -lapue_db -lapue

t4dump:	$(LIBAPUE)
		$(CC) $(CFLAGS) -c -I. t4dump.c
		$(CC) $(EXTRALD) -o t4dump t4dump.o -L$(ROOT)/lib -L. -lapue_db -lapue

clean:
	rm -f *.o a.out core temp.* $(LIBMISC) t4 t4dump libapue_db.so.* *.dat *.idx \
	libapue_db.so

include $(ROOT)/Make.libapue.inc
