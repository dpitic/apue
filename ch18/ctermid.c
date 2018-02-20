/*
 * This function demonstrates an implementation of the POSIX.1 ctermid()
 * function.
 */
#include <stdio.h>
#include <string.h>

static char ctermid_name[L_ctermid];

/**
 * Return a string that is the pathname for the current controlling terminal
 * for this process.
 * @param str if NULL, a static buffer is used, otherwise str points to a buffer
 * used to hold the terminal pathname.  The symbolic constant L_ctermid is the
 * maximum number of characters int he returned pathname.
 * @return pointer to the pathname.
 */
char *ctermid(char *str) {
  if (str == NULL) {
    str = ctermid_name;
  }
  return (strcpy(str, "/dev/tty")); /* strcpy() returns str */
}
