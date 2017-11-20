/*
 * This program demonstrates the use of pthread_atfork() and fork handlers.
 */
#include "apue.h"
#include <pthread.h>

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;

void prepare(void) {
  int err;

  printf("Preparing locks...\n");
  if ((err = pthread_mutex_lock(&lock1)) != 0) {
    err_cont(err, "Can't lock lock1 in prepare handler");
  }
  if ((err = pthread_mutex_lock(&lock2)) != 0) {
    err_cont(err, "Can't lock lock2 in prepare handler");
  }
}

void parent(void) {
  int err;

  printf("Parent unlocking locks...\n");
  if ((err = pthread_mutex_unlock(&lock1)) != 0) {
    err_cont(err, "Can't unlock lock1 in parent handler");
  }
  if ((err = pthread_mutex_unlock(&lock2)) != 0) {
    err_cont(err, "Can't unlock lock2 in child handler");
  }
}

void child(void) {
  int err;

  printf("Child unlocking locks...\n");
  if ((err = pthread_mutex_unlock(&lock1)) != 0) {
    err_cont(err, "Can't unlock lock1 in child handler");
  }
  if ((err = pthread_mutex_unlock(&lock2)) != 0) {
    err_cont(err, "Can't unlock lock2 in child handler");
  }
}

void *thr_fn(void *arg) {
  printf("Thread started...\n");
  pause();
  return (0);
}

int main(void) {
  int err;
  pid_t pid;
  pthread_t tid;

  if ((err = pthread_atfork(prepare, parent, child)) != 0) {
    err_exit(err, "Can't install fork handlers");
  }
  if ((err = pthread_create(&tid, NULL, thr_fn, 0)) != 0) {
    err_exit(err, "Can't create thread");
  }

  sleep(2);
  printf("Parent about to fork...\n");

  if ((pid = fork()) < 0) {
    err_quit("fork failed");
  } else if (pid == 0) { /* child */
    printf("Child returned from fork\n");
  } else { /* parent */
    printf("Parent returned from fork\n");
  }
  exit(0);
}