/*
 * Copy from standard input to standard output. This program is implemented
 * using the standard library I/O functions getc() and putc(). These functions
 * operate on individual characters in the stream. They may be implemented in
 * the standard library as macros.
 */
#include "apue.h"

int main(void) {
  int c;

  while ((c = getc(stdin)) != EOF) {
    if (putc(c, stdout) == EOF) {
      err_sys("output error");
    }
  }

  if (ferror(stdin)) {
    err_sys("input error");
  }
  exit(0);
}