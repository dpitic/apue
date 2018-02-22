/*
 * This program demonstrates the use of the getpass() function, and prints what
 * the user enters, to enalbe verification that the ERASE and KILL characters
 * work (as they should in canonical mode).
 */
#include "apue.h"

/* Custom getpass() prototype */
char *getpass(const char *);

int main(void) {
  char *ptr;

  if ((ptr = getpass("Enter password: ")) == NULL) {
    err_sys("getpass() error");
  }

  /* Print password, for test purposes only */
  printf("password: %s\n", ptr);

  /* Now use password (probably encrypt it) ... */

  while (*ptr != 0) {
    *ptr++ = 0; /* zero it out when finished with password */
  }
  exit(0);
}
