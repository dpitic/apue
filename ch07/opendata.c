/*
 * This function opens a standard I/O stream and sets the buffering for the
 * stream.  The problem is that when the function returns, the space it used
 * on the stack will be used by the stack frame for the next function that is
 * called, but the standard I/O library will still be using that portion of
 * memory for its stream buffer.  To correct this problem, the array databuf
 * needs to be allocated from global memory, either statically (static or
 * extern) or dynamically (one of the alloc() functions).
 */
#include <stdio.h>

FILE *open_data(void) {
  FILE *fp;
  char databuf[BUFSIZ]; /* setvbuf makes this the stdio buffer */

  if ((fp = fopen("datafile", "r")) == NULL) {
    return (NULL);
  }
  /* Make file stream line buffered and databuf the stdio buffer */
  if (setvbuf(fp, databuf, _IOLBF, BUFSIZ) != 0) {
    return (NULL);
  }
  return (fp); /* error; fp points to databuf on the stack; not persistent  */
}