/*
 * This program prints out the current soft and hard limits for all of the
 * resource limits supported on the system.  To compile this program on all the
 * various implementations, the program has to conditionally include the
 * resource names that differ.
 *
 * Some systems define rlim_t to be unsigned long long instead of unsigned long.
 * This definition an even change on the same system, depending on whether the
 * program is compiled to support 64-bit files.  Some limits apply to file size,
 * so the rlim_t type has to be large enough to represent a file size limit.
 * To avoid compiler warnings that use the wrong format specification, the limit
 * is first copied into a 64-bit integer so that the program only has to deal
 * with one format.
 */
#include "apue.h"
#include <sys/resource.h>

#define doit(name) pr_limits(#name, name)

static void pr_limits(char *, int);

int main(void) {
#ifdef RLIMIT_AS
  doit(RLIMIT_AS);
#endif

  doit(RLIMIT_CORE);
  doit(RLIMIT_CPU);
  doit(RLIMIT_DATA);
  doit(RLIMIT_FSIZE);

#ifdef RLIMIT_MEMLOCK
  doit(RLIMIT_MEMLOCK);
#endif

#ifdef RLIMIT_MSGQUEUE
  doit(RLIMIT_MSGQUEUE);
#endif

#ifdef RLIMIT_NICE
  doit(RLIMIT_NICE);
#endif

  doit(RLIMIT_NOFILE);

#ifdef RLIMIT_NPROC
  doit(RLIMIT_NPROC);
#endif

#ifdef RLIMIT_NPTS
  doit(RLILMIT_NPTS);
#endif

#ifdef RLIMIT_RSS
  doit(RLIMIT_RSS);
#endif

#ifdef RLIMIT_SBSIZE
  doit(RLIMIT_SBSIZE);
#endif

#ifdef RLIMIT_SIGPENDING
  doit(RLIMIT_SIGPENDING);
#endif

  doit(RLIMIT_STACK);

#ifdef RLIMIT_SWAP
  doit(RLIMIT_SWAP);
#endif

#ifdef RLIMIT_VMEM
  doit(RLIMIT_VMEM);
#endif

  exit(0);
}

/* Print system limit */
static void pr_limits(char *name, int resource) {
  struct rlimit limit;
  unsigned long long lim;

  if (getrlimit(resource, &limit) < 0) {
    err_sys("getrlimit error for %s", name);
  }
  printf("%-18s  ", name);
  if (limit.rlim_cur == RLIM_INFINITY) {
    printf("(infinite)  ");
  } else {
    lim = limit.rlim_cur;
    printf("%10lld  ", lim);
  }
  if (limit.rlim_max == RLIM_INFINITY) {
    printf("(infinite)");
  } else {
    lim = limit.rlim_max;
    printf("%10lld", lim);
  }
  putchar((int)'\n');
}