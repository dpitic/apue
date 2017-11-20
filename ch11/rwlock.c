/*
 * This program demonstrates the use of reader-writer locks.  It implements a
 * queue of job requests protected by a single reader-writer lock.  It shows a
 * possible implementation where multiple worker threads obtain jobs assigned
 * to them by a single master thread.
 */
#include <pthread.h>
#include <stdlib.h>

/* 
 * Job structure.  The intent is that worker threads only process jobs they are
 * assigned to (jobs that match the thread ID).  Therefore, since job objects
 * are used only by one thread at a time, they don't need any extra locking.
 */
struct job {
  struct job *j_next;
  struct job *j_prev;
  pthread_t j_id; /* job ID tracks which thread handles this job */
  /* ... more struct members here ... */
};

/* Job queue structure.  This is designed to be accessed by multiple threads. */
struct queue {
  struct job *q_head;
  struct job *q_tail;
  pthread_rwlock_t q_lock;
};

/*
 * Initialise a job queue.  Return 0 on success or err on failure.
 */
int queue_init(struct queue *qp) {
  int err;

  qp->q_head = NULL;
  qp->q_tail = NULL;
  err = pthread_rwlock_init(&qp->q_lock, NULL);
  if (err != 0) {
    return (err);
  }
  /* ... continue initialisation ... */
  return (0);
}

/*
 * Insert a job at the head of the job queue.
 */
void job_insert(struct queue *qp, struct job *jp) {
  pthread_rwlock_wrlock(&qp->q_lock);
  jp->j_next = qp->q_head;
  jp->j_prev = NULL;
  if (qp->q_head != NULL) {
    qp->q_head->j_prev = jp;
  } else {
    qp->q_tail = jp; /* the list was empty */
  }
  qp->q_head = jp;
  pthread_rwlock_unlock(&qp->q_lock);
}

/*
 * Append a job on the tail of the job queue.
 */
void job_append(struct queue *qp, struct job *jp) {
  pthread_rwlock_wrlock(&qp->q_lock);
  jp->j_next = NULL;
  jp->j_prev = qp->q_tail;
  if (qp->q_tail != NULL) {
    qp->q_tail->j_next = jp;
  } else {
    qp->q_head = jp; /* list was empty */
  }
  qp->q_tail = jp;
  pthread_rwlock_unlock(&qp->q_lock);
}

/*
 * Remove the given job from the job queue.
 */
void job_remove(struct queue *qp, struct job *jp) {
  pthread_rwlock_wrlock(&qp->q_lock);
  if (jp == qp->q_head) {
    qp->q_head = jp->j_next;
    if (qp->q_tail == jp) {
      qp->q_tail = NULL;
    } else {
      jp->j_next->j_prev = jp->j_prev;
    }
  } else if (jp == qp->q_tail) {
    qp->q_tail = jp->j_prev;
    jp->j_prev->j_next = jp->j_next;
  } else {
    jp->j_prev->j_next = jp->j_next;
    jp->j_next->j_prev = jp->j_prev;
  }
  pthread_rwlock_unlock(&qp->q_lock);
}

/*
 * Find a job for the given thread ID.  Return pointer to struct job if found or
 * NULL if not found.
 */
struct job *job_find(struct queue *qp, pthread_t id) {
  struct job *jp;

  if (pthread_rwlock_rdlock(&qp->q_lock) != 0) {
    return (NULL);
  }

  for (jp = qp->q_head; jp != NULL; jp = jp->j_next) {
    if (pthread_equal(jp->j_id, id)) {
      break;
    }
  }

  pthread_rwlock_unlock(&qp->q_lock);
  return (jp);
}