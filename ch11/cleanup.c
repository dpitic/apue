/*
 * This program demonstrates how to use thread cleanup handlers.
 */
#include "apue.h"
#include <pthread.h>

/* Thread cleanup handler */
void cleanup(void *arg) { printf("cleanup: %s\n", (char *)arg); }

/*
 * Thread 1 start function.  This function will return, therefore the thread
 * cleanup handlers will not be called.  Also in SUS v4, returning while in
 * between a matched pair of calls to pthread_cleanup_push() and
 * pthread_cleanup_pop() results in undefined behaviour.  The only portable way
 * to return in between these two functions is to call pthread_exit().
 */
void *thr_fn1(void *arg) {
  printf("thread 1 start\n");
  pthread_cleanup_push(cleanup, "thread 1 first handler");
  pthread_cleanup_push(cleanup, "thread 1 second handler");
  printf("thread 1 push complete\n");
  if (arg) {
    return ((void *)1);
  }
  pthread_cleanup_pop(0);
  pthread_cleanup_pop(0);
  return ((void *)1);
}

/*
 * Thread 2 start function.  This function calls pthread_exit(), so the thread
 * cleanup functions will be called.  This function demonstrates the only
 * portable way of returning in between pthread_cleanup_push() and
 * pthread_cleanup_pop() functions is by calling pthread_exit().
 */
void *thr_fn2(void *arg) {
  printf("thread 2 start\n");
  pthread_cleanup_push(cleanup, "thread 2 first handler");
  pthread_cleanup_push(cleanup, "thread 2 second handler");
  printf("thread 2 push complete\n");
  if (arg) {
    pthread_exit((void *)2);
  }
  pthread_cleanup_pop(0);
  pthread_cleanup_pop(0);
  pthread_exit((void *)2);
}

int main(void) {
  int err;
  pthread_t tid1, tid2;
  void *tret;

  err = pthread_create(&tid1, NULL, thr_fn1, (void *)1);
  if (err != 0) {
    err_exit(err, "can't create thread 1");
  }
  err = pthread_create(&tid2, NULL, thr_fn2, (void *)1);
  if (err != 0) {
    err_exit(err, "can't create thread 2");
  }

  err = pthread_join(tid1, &tret);
  if (err != 0) {
    err_exit(err, "can't join with thread 1");
  }
  printf("thread 1 exit code %ld\n", (long)tret);

  err = pthread_join(tid2, &tret);
  if (err != 0) {
    err_exit(err, "can't join with thread 2");
  }
  printf("thread 2 exit code %ld\n", (long)tret);
  exit(0);
}
