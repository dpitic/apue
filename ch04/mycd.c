/*
 * The current working directory is an attribute of a process and cannot affect
 * processes that invoke the process that executes the chdir() function. As a
 * result, this program doesn't do what it might be expected to do.
 *
 * The current working directory for the shell that executes this program won't
 * change, because this program is run in its own process, separate to the
 * shell process.
 *
 * For this reason, the cd command has to be built into the shell.
 */
#include "apue.h"

int main(int argc, char const *argv[]) {
  if (chdir("/tmp") < 0) {
    err_sys("chdir failed");
  }
  printf("chdir to /tmp succeeded\n");
  exit(0);
}
