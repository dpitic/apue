/*
 * This program demonstrates the fork() function, showing how changes to
 * variables in a child process do not affect the value of variables in the
 * parent process.
 */
#include "apue.h"

int globvar = 7; /* external variable in initialised data segment */
char buf[] = "a write to stdout\n";

int main(void) {
  int var;      /* automatic variable on the stack */
  pid_t pid;

  var = 88;
  if (write(STDOUT_FILENO, buf, sizeof(buf) - 1) != sizeof(buf) - 1) {
    err_sys("write error");
  }
  printf("before fork\n"); /* we don't flush stdout */

  if ((pid = fork()) < 0) {
    err_sys("fork error");
  } else if (pid == 0) {    /* child */
    globvar++;              /* modify variables */
    var++;
  } else {
    /* 
     * Put parent to sleep to let the child execute; execution order is
     * indeterminate
     */
    sleep(2);               /* parent */
  }

  /* 
   * Both parent and child run this code.  Parent's copy of variables are not 
   * changed 
   */
  printf("pid = %ld, glob = %d, var = %d\n", (long)getpid(), globvar, var);
  exit(0);
}