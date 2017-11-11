/*
 * This program demonstrates the use of the pthread_mutex_timedlock() function.
 * This function blocks for the specified absolute time.
 */
#include "apue.h"
#include <pthread.h>

int main(void) {
  int err;
  struct timespec tout;
  struct tm *tmp;
  char buf[64];
  pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

  pthread_mutex_lock(&lock);
  printf("mutex is locked\n");
  clock_gettime(CLOCK_REALTIME, &tout);
  tmp = localtime(&tout.tv_sec);
  strftime(buf, sizeof(buf), "%r", tmp);
  printf("current time is %s\n", buf);
  tout.tv_sec += 10; /* 10 seconds from now */
  /* 
   * Caution: this could lead to a deadlock
   * This program deliberately locks a mutex it already owns to demonstrate how
   * pthread_mutex_timedlock() works.  Not recommended in practice.
   */
  err = pthread_mutex_timedlock(&lock, &tout);
  clock_gettime(CLOCK_REALTIME, &tout);
  tmp = localtime(&tout.tv_sec);
  strftime(buf, sizeof(buf), "%r", tmp);
  printf("the time is now %s\n", buf);
  if (err == 0) {
    printf("mutex locked again!\n");
  } else {
    printf("can't lock mutex again: %s\n", strerror(err));
  }
  exit(0);
}