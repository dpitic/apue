/*
 * This program copies a file using memory-mapped I/O.  It is similar to the
 * cp(1) command.
 */
#include <apue.h>
#include <fcntl.h>
#include <sys/mman.h>

/* Copy buffer increment maximum size; used to limit amount of memory used */
#define COPYINCR (1024 * 1024 * 1024) /* 1 GB */

int main(int argc, char *argv[]) {
  int fdin, fdout;  /* input and output file descriptors */
  void *src, *dst;  /* source and destination memory map pointers */
  size_t copysz;    /* buffer size to copy */
  struct stat sbuf; /* input file */
  off_t fsz = 0;    /* file descriptor byte offset */

  if (argc != 3) {
    err_quit("Usage: %s <fromfile> <tofile>", argv[0]);
  }
  if ((fdin = open(argv[1], O_RDONLY)) < 0) {
    err_sys("Can't open %s for reading", argv[1]);
  }
  if ((fdout = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, FILE_MODE)) < 0) {
    err_sys("Can't creat %s for writing", argv[2]);
  }
  if (fstat(fdin, &sbuf) < 0) { /* determine size of input file */
    err_sys("fstat() error");
  }
  if (ftruncate(fdout, sbuf.st_size) < 0) { /* set output file size */
    err_sys("ftruncate() error");
  }

  while (fsz < sbuf.st_size) {
    if ((sbuf.st_size - fsz) > COPYINCR) {
      copysz = COPYINCR; /* maximum copy size buffer */
    } else {
      copysz = sbuf.st_size - fsz;
    }

    /* Map both input and output files into memory */
    if ((src = mmap(0, copysz, PROT_READ, MAP_SHARED, fdin, fsz)) ==
        MAP_FAILED) {
      err_sys("mmap() error for input");
    }
    if ((dst = mmap(0, copysz, PROT_READ | PROT_WRITE, MAP_SHARED, fdout,
                    fsz)) == MAP_FAILED) {
      err_sys("mmap() error for output");
    }

    memcpy(dst, src, copysz); /* perform the file copy */
    /* Unmap current memory sections */
    munmap(src, copysz);
    munmap(dst, copysz);
    /* Increment descriptor offset for next sections to copy */
    fsz += copysz;
  }
  exit(0);
}
