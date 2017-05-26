/*
 * This program echoes all its command-line arguments to standard output. When
 * a program is executed, the process that does the exec() can pass command
 * line arguments to the new program. This is part of the normal operation of
 * the UNIX system shells.
 */
#include "apue.h"

int main(int argc, char *argv[]) {
  int i;

  for (i = 0; i < argc; i++) {
    printf("argv[%d]: %s\n", i, argv[i]);
  }

  /* ISO C and POSIX.1 guarantees that argv[argc] is a null pointer; so the
   * alternative code can be written.
   */
  for (i = 0; argv[i] != NULL; i++) {
    printf("argv[%d]: %s\n", i, argv[i]);
  }
  exit(0);
}