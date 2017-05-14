/*
 * This program prints the buffering for the three standard streams and for a
 * stream that is associated with a regular file.
 */
#include "apue.h"

void pr_stdio(const char *, FILE *);
int is_unbuffered(FILE *);
int is_linebuffered(FILE *);
int buffer_size(FILE *);

int main(void) {
  FILE *fp;

  /* 
   * Perform I/O on each stream before printing its buffering status. The first
   * I/O operation usually causes the buffers to be allocated for a stream.
   */
  fputs("Enter any character\n", stdout);
  if (getchar() == EOF) {
    err_sys("getchar error");
  }

  fputs("One line to standard error\n", stderr);

  pr_stdio("stdin", stdin);
  pr_stdio("stdout", stdout);
  pr_stdio("stderr", stderr);

  if ((fp = fopen("/etc/passwd", "r")) == NULL) {
    err_sys("fopen error");
  }
  if (getc(fp) == EOF) {
    err_sys("getc error");
  }
  pr_stdio("/etc/passwd", fp);
  exit(0);
}

void pr_stdio(const char *name, FILE *fp) {
  printf("stream = %s, ", name);
  if (is_unbuffered(fp)) {
    printf("unbuffered");
  } else if (is_linebuffered(fp)) {
    printf("line buffered");
  } else {
    printf("fully buffered");
  }
  printf(", buffer size = %d\n", buffer_size(fp));
}

/*
 * The following is not portable code.
 */
#if defined(_IO_UNBUFFERED)

int is_unbuffered(FILE *fp) { return (fp->_flags & _IO_UNBUFFERED); }

int is_linebuffered(FILE *fp) { return (fp->_flags & _IO_LINE_BUF); }

int buffer_size(FILE *fp) { return (fp->_IO_buf_end - fp->_IO_buf_base); }

#elif defined(__SNBF)

int is_unbuffered(FILE *fp) { return (fp->_flags & __SNBF); }

int is_linebuffered(FILE *fp) { return (fp->_flags & __SLBF); }

int buffer_size(FILE *fp) { return (fp->_bf._size); }

#elif defined(_IONBF)

#ifdef _LP64
#define _flag __pad[4]
#define _prt __pad[1]
#define _base __pad[2]
#endif

int is_unbuffered(FILE *fp) { return (fp->_flag & _IONBF); }

int is_linebuffered(FILE *fp) { return (fp->_flag & _IOLBF); }

int buffer_size(FILE *fp) {
#ifdef _LP64
  return (fp->_base - fp->_ptr);
#else
  return (BUFSIZ); /* just a guess */
#endif
}

#else

#error unknown stdio implementation!

#endif