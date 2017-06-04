/*
 * Echo all command-line arguments and all env strings.
 */
#include "apue.h"

int main(int argc, char *argv[]) {
  int i;
  char **ptr;
  extern char **environ;

  /* echo all command-line arguments */
  for (i = 0; i < argc; i++) {
    printf("argv[%d]: %s\n", i, argv[i]);
  }

  /* echo all env strings */
  for (ptr = environ; *ptr != 0; ptr++) {
    printf("%s\n", *ptr);
  }

  exit(0);
}