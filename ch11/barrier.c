/*
 * This program demonstrates the use of a barrier to synchronise threads
 * cooperating on a single task.
 */
#include "apue.h"
#include <limits.h>
#include <pthread.h>
#include <sys/time.h>

#define NTHR 8               /* number of threads */
#define NUMNUM 8000000L      /* number of numbers to sort */
#define TNUM (NUMNUM / NTHR) /* number of sort per thread */

long nums[NUMNUM];  /* unsorted array */
long snums[NUMNUM]; /* sorted array */

pthread_barrier_t b;

#ifdef SOLARIS
#define heapsort qsort
#else
extern int heapsort(void *, size_t, size_t,
                    int (*)(const void *, const void *));
#endif

/*
 * Compare two long integers (helper function for heapsort())
 */
int complong(const void *arg1, const void *arg2) {
  long l1 = *(long *)arg1;
  long l2 = *(long *)arg2;

  if (l1 == l2) {
    return 0;
  } else if (l1 < l2) {
    return -1;
  } else {
    return 1;
  }
}

/*
 * Worker thread to sort a portion of the set of numbers.
 */
void *thr_fn(void *arg) {
  long idx = (long)arg; /* array start index for this thread */

  heapsort(&nums[idx], TNUM, sizeof(long), complong);
  pthread_barrier_wait(&b);

  /*
   * Go off and perform more work ...
   */
  return ((void *)0);
}

/*
 * Merge the results of the individual sorted ranges.
 */
void merge() {
  long idx[NTHR]; /* thread start index array */
  long i, minidx, sidx, num;

  /* Calculate thread start indices */
  for (i = 0; i < NTHR; i++) {
    idx[i] = i * TNUM;
  }

  /* Merge loop for sorted array */
  for (sidx = 0; sidx < NUMNUM; sidx++) {
    num = LONG_MAX;

    /* Sort the sorted arrays */
    for (i = 0; i < NTHR; i++) {
      if ((idx[i] < (i + 1) * TNUM) && (nums[idx[i]] < num)) {
        num = nums[idx[i]];
        minidx = i;
      }
    }
    snums[sidx] = nums[idx[minidx]];
    idx[minidx]++;
  }
}

int main() {
  unsigned long i;
  struct timeval start, end;
  long long startusec, endusec;
  double elapsed;
  int err;
  pthread_t tid;

  /*
   * Create the initial set of numbers to sort.
   */
  printf("Creating array of random numbers to sort ...\n");
  srandom(1);
  for (i = 0; i < NUMNUM; i++) {
    nums[i] = random();
  }

  /*
   * Create NTHR threads to sort the numbers.
   */
  printf("Sorting array ...\n");
  gettimeofday(&start, NULL);
  /* 
   * The main thread will be used to merge the sorted sub-arrays, so there is
   * no need to use the PTHREAD_BARRIER_SERIAL_THREAD return value from
   * pthread_barrier_wait() to decide which thread merges the results.  The
   * barrier thread count is specified as one more than the number of worker
   * threads; the main thread counts as one waiter thread.
   */
  pthread_barrier_init(&b, NULL, NTHR + 1);
  for (i = 0; i < NTHR; i++) {
    /* Unsorted array thread start index given by last argument */
    err = pthread_create(&tid, NULL, thr_fn, (void *)(i * TNUM));
    if (err != 0) {
      err_exit(err, "can't create thread");
    }
  }
  pthread_barrier_wait(&b); /* wait for all threads to finish sorting */
  merge();
  gettimeofday(&end, NULL);

  /*
   * Print the sorted list.
   */
  startusec = start.tv_sec * 1000000 + start.tv_usec;
  endusec = end.tv_sec * 1000000 + end.tv_usec;
  elapsed = (double)(endusec - startusec) / 1000000.0;
  printf("Sort took %.4f seconds\n", elapsed);
  /*
  for (i = 0; i < NUMNUM; i++) {
    printf("%ld\n", snums[i]);
  }
  */

  /* Print a selection of the start and end of the sorted array */
  for (i = 0; i < 5; i++) {
    printf("%ld\n", snums[i]);
  }
  printf("...\n");
  for (i = NUMNUM - 5; i < NUMNUM; i++) {
    printf("%ld\n", snums[i]);
  }
  exit(0);
}