/*
 * This program is a modified version of fork1 using vfork().  We don't have to
 * call sleep in the parent because the parent is guaranteed to sleep by the
 * kernel until the child calls either exec or exit.
 */
#include "apue.h"

int globvar = 6; /* external variable in initialised data segment */

int main(void) {
  int var; /* automatic variable on the stack */
  pid_t pid;

  var = 88;
  printf("before vfork\n");     /* we don't flush stdio */
  if ((pid = vfork()) < 0) {
    err_sys("vfork error");
  } else if (pid == 0) {    /* child */
    globvar++;              /* modify parent's variables */
    var++;
    _exit(0);               /* child terminates */
  }

  /* 
   * Parent continues here; child changes the values in the parent because the
   * child runs in the address space of the parent.  This is different behaviour
   * than fork().
   */
  printf("pid = %ld, glob = %d, var = %d\n", (long)getpid(), globvar, var);
  exit(0);
}