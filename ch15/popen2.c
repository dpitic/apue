/*
 * This program demonstrates an alternative implementation of pipe2 using the
 * popen() function, which reduces the amount of code that has to be written.
 * Like before, it outputs the given file in the pager.
 */
#include "apue.h"
#include <sys/wait.h>

/* Pager environment variable, or default pager, more */
#define PAGER "${PAGER:-more}"

int main(int argc, char *argv[])
{
  char line[MAXLINE];
  FILE *fpin, *fpout;

  if (argc != 2) {
    err_quit("Usage: %s <pathname>", argv[0]);
  }

  if ((fpin = fopen(argv[1], "r")) == NULL) {
    err_sys("Can't open %s", argv[1]);
  }

  if ((fpout = popen(PAGER, "w")) == NULL) {
    err_sys("popen() error");
  }

  /* Copy argv[1] to pager */
  while (fgets(line, MAXLINE, fpin) != NULL) {
    if (fputs(line, fpout) == EOF) {
      err_sys("fputs() error to pipe");
    }
  }

  if (ferror(fpin)) {
    err_sys("fgets() error");
  }
  if (pclose(fpout) == -1) {
    err_sys("pclose() error");
  }

  exit(0);
}
