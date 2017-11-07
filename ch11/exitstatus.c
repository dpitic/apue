/*
 * This program demonstrates how to fetch the exit code from a thread that has
 * terminated.
 */
#include "apue.h"
#include <pthread.h>

/* Thread 1 start function */
void *thr_fn1(void *arg) {
  printf("thread 1 returning\n");
  return ((void *)1);
}

/* Thread 2 start function */
void *thr_fn2(void *arg) {
  printf("thread 2 exiting\n");
  pthread_exit((void *)2);
}

int main(void) {
  int err;
  pthread_t tid1, tid2;
  void *tret;

  /* Create 2 threads to demonstrate thread joining */
  err = pthread_create(&tid1, NULL, thr_fn1, NULL);
  if (err != 0) {
    err_exit(err, "can't create thread 1");
  }
  
  err = pthread_create(&tid2, NULL, thr_fn2, NULL);
  if (err != 0) {
    err_exit(err, "can't create thread 2");
  }

  /* This thread will block until thread 1 returns */
  err = pthread_join(tid1, &tret);
  if (err != 0) {
    err_exit(err, "can't join with thread 1");
  }
  printf("thread 1 exit code %ld\n", (long)tret);

  /* This thread will block until thread 2 calls pthread_exit() */
  err = pthread_join(tid2, &tret);
  if (err != 0) {
    err_exit(err, "can't joint with thread 2");
  }
  printf("thread 2 exit code %ld\n", (long)tret);
  exit(0);
}