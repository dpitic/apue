/*
 * This program demonstrates the standard I/O library functions tempnam() and
 * tmpfile(). These functions should not be used; use mkstemp(3) or
 * tmpfile(3) instead.
 *
 * The problem with tmpnam() and tempnam() is that a window exists between the
 * time that the unique pathname is returned and the time that an application
 * creates a file with that name. During this time window, another process can
 * create a file of the same name.
 */
#include "apue.h"

int main(void) {
  char name[L_tmpnam], line[MAXLINE];
  FILE *fp;

  printf("%s\n", tmpnam(NULL));       /* first temp name */

  tmpnam(name);                       /* second temp name */
  printf("%s\n", name);

  if ((fp = tmpfile()) == NULL) {     /* create temp file */
    err_sys("tmpfile error");
  }
  fputs("one line of output\n", fp);  /* write to temp file */
  rewind(fp);                         /* then read it back */
  if (fgets(line, sizeof(line), fp) == NULL) {
    err_sys("fgets error");
  }
  fputs(line, stdout);                /* print the line we wrote */

  exit(0);
}
