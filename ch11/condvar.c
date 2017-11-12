/*
 * This program demonstrates how to use a condition variable and a mutex
 * together to synchronise threads.
 */
#include <pthread.h>

/* Shared structure */
struct msg {
  struct msg *m_next;
  /* ... more members here ... */
};

struct msg *workq;

/* Condition variable is the state of the work queue */
pthread_cond_t qready = PTHREAD_COND_INITIALIZER;
/* Condition is protected with a mutex */
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;

void process_msg(void) {
  struct msg *mp;

  for (;;) {
    pthread_mutex_lock(&qlock);
    while (workq == NULL) {
      pthread_cond_wait(&qready, &qlock);
    }
    mp = workq;
    workq = mp->m_next;
    pthread_mutex_unlock(&qlock);
    /* Now process the message mp */
  }
}

void enqueue_msg(struct msg *mp) {
  pthread_mutex_lock(&qlock);
  mp->m_next = workq;
  workq = mp;
  pthread_mutex_unlock(&qlock);
  pthread_cond_signal(&qready);
}