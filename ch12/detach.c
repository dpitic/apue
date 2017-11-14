/*
 * This function demonstrates how to create a thread in the detached state.
 */
#include "apue.h"
#include <pthread.h>

/*
 * Create a thread in the detached state.  Return 0 on success or error number
 * on failure.
 */
int makethread(void *(fn)(void *), void *arg) {
  int err;
  pthread_t tid;
  pthread_attr_t attr;

  err = pthread_attr_init(&attr);
  if (err != 0) {
    return (err);
  }
  err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (err == 0) {
    err = pthread_create(&tid, &attr, fn, arg);
  }
  pthread_attr_destroy(&attr);
  return (err);
}