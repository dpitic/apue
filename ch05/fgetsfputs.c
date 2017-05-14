/*
 * Copy from standard input to standard output. This program is implemented
 * using the standard library I/O functions fgets() and fputs(). These functions
 * operate on line buffers.
 */
#include "apue.h"

int main(void) {
  char buf[MAXLINE];

  while (fgets(buf, MAXLINE, stdin) != NULL) {
    if (fputs(buf, stdout) == EOF) {
      err_sys("output error");
    }
  }

  if (ferror(stdin)) {
    err_sys("input error");
  }
  exit(0);
}