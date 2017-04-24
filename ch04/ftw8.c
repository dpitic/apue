/*
 * This program uses directory functions to traverse a file hirearchy to produce
 * a count of the various types of files. It takes a single argument, the
 * starting pathname, and recursively descends the hierarchy from that point.
 *
 * Solaris provides the function ftw(3) that performs the actual traversal of
 * the hierarchy, calling a user-defined function for each file. The function
 * ftw(3) calls the stat() function for each file, which causes the program to
 * follow symbolic links. This means that symbolic links may cause files to be
 * counted twice. To correct this problem, Solaris provides another function
 * called nftw(3), with an option that stops it from following symbolic links.
 *
 * Although we could use nftw(3), the program implements its own file tree
 * walker to demonstrate the use of the directory system calls.
 */
#include "apue.h"
#include <dirent.h>
#include <limits.h>

/* function type that is called for each filename */
typedef int Myfunc(const char *, const struct stat *, int);

static Myfunc myfunc;
static int myftw(const char *, Myfunc *);
static int dopath(Myfunc *);

static long nreg, ndir, nblk, nchr, nfifo, nslink, nsock, ntot;

int main(int argc, char const *argv[]) {
  int ret;

  if (argc != 2) {
    err_quit("usage: %s <starting-pathname>", argv[0]);
  }

  ret = myftw(argv[1], myfunc); /* this does it all */

  ntot = nreg + ndir + nblk + nchr + nfifo + nslink + nsock;
  if (ntot == 0) {
  	ntot = 1;	/* avoid divide by 0; print 0 for all counts */
  }
  printf("regular files  = %7ld, %5.2f %%\n", nreg, nreg * 100.0 / ntot);
  printf("directories    = %7ld, %5.2f %%\n", ndir, ndir * 100.0 / ntot);
  printf("block special  = %7ld, %5.2f %%\n", nblk, nblk * 100.0 / ntot);
  printf("char special   = %7ld, %5.2f %%\n", nchr, nchr * 100.0 / ntot);
  printf("FIFOs          = %7ld, %5.2f %%\n", nfifo, nfifo * 100.0 / ntot);
  printf("symbolic links = %7ld, %5.2f %%\n", nslink, nslink * 100.0 / ntot);
  printf("sockets        = %7ld, %5.2f %%\n", nsock, nsock * 100.0 / ntot);
  exit(ret);
}

/*
 * Descend through the hierarchy, starting at pathname. The caller's func() is
 * called for every file.
 */
#define FTW_F	1	/* file other than directory */
#define FTW_D	2	/* directory */
#define FTW_DNR 3	/* directory that can't be read */
#define FTW_NS	4	/* file that we can't stat */

static char *fullpath;	/* contains full pathname for every file */
static size_t pathlen;

static int myftw(const char *pathname, Myfunc *func) {
	fullpath = path_alloc(&pathlen);
}