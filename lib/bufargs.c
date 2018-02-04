#include "apue.h"

#define MAXARGC 50    /* max number of arguments in buf */
#define WHITE " \t\n" /* white space for tokenizing arguments */

/**
 * Breaks null-terminated string of white space separated arguments into a
 * standard argv style argument list and calls a user function to process the
 * arguments.
 * @param buf string containing the white space separated arguments to be
 * tokenized and converted into an argv style array of pointers which the
 * user function is used to process.
 * @param optfunc user defined function that is used to process the arguments.
 * @return return value of user function, on success; -1 on error.
 */
int buf_args(char *buf, int (*optfunc)(int, char **)) {
  char *ptr, *argv[MAXARGC];
  int argc;

  if (strtok(buf, WHITE) == NULL) { /* an argv[0] is required */
    return (-1);
  }
  argv[argc = 0] = buf;
  while ((ptr = strtok(NULL, WHITE)) != NULL) {
    if (++argc >= MAXARGC - 1) { /* -1 for room for NULL at end */
      return (-1);
    }
    argv[argc] = ptr;
  }
  argv[++argc] = NULL;

  /*
   * Since argv[] pointers point into the user's buf[], user's function can
   * just copy the pointers, even though argv[] array will disappear on
   * return.
   */
  return ((*optfunc)(argc, argv));
}
