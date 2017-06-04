/*
 * This program demonstrates what the kernel does with the arguments to the
 * exec function when the file being executed is an interpreter file and the
 * optional argument on the first line of the interpreter file.  This program
 * execs an interpreter file.
 */
#include "apue.h"
#include <sys/wait.h>

int main(void) {
  pid_t pid;

  if ((pid = fork()) < 0) {
    err_sys("fork error");
  } else if (pid == 0) {
    if (execl("./testinterp", "testinterp", "myarg1", "MY ARG2", (char *)0)
        < 0) {
      err_sys("execl error");
    }
  }
  if (waitpid(pid, NULL, 0) < 0) {
    err_sys("waitpid error");
  }
  exit(0);
}