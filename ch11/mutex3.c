/*
 * This program simplifies the complicated locking approach used in mutex2 by
 * using the hash list lock to protect the structure reference count also.  The
 * lock ordering issues surrounding the hash list and the reference count are
 * resolved by using the same lock for both purposes.  As previously, the struct
 * mutex is used to protect the rest of the struct members.  The design 
 * trade-offs in multithreaded software design mean that if the locking
 * granularity is too coarse, the results is that too many thread block behind
 * the same locks, with little improvement from concurrency.  If locking
 * granularity is too fine, the performance suffers from excessive locking
 * overhead and the complexity of the code increases.  Therefore, a balance has
 * to be achieved betwen code complexity and performance, while still satisfying
 * the locking requirements.
 */
#include <pthread.h>
#include <stdlib.h>

#define NHASH 29
#define HASH(id) (((unsigned long)id) % NHASH)

/* Hash list used to store the foo data structs */
struct foo *fh[NHASH];

/* 
 * Mutex used to protect the hash list, the f_next hash link field and the 
 * reference counter.  This is a statically allocated mutex, therefore must be
 * initialised as shown.
 */
pthread_mutex_t hashlock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Sample thread safe struct.  Only thread safe when manipulated by the
 * functions in this file.  This time, hashlock is also used to protect the
 * object reference counter.
 */
struct foo {
  int f_count;            /* object reference counter; protected by hashlock */
  pthread_mutex_t f_lock; /* mutex lock for this object */
  int f_id;               /* object id */
  /* Pointer to next element in the list; protected by hashlock */
  struct foo *f_next;
  /* ... other struct members here ... */
};

/*
 * Object memory allocation and initialisation function.  Returns new struct foo
 * object on success, or NULL on failure.
 */
struct foo *foo_alloc(int id) {
  struct foo *fp;
  int idx; /* hash list index */

  /* Allocate memory and initialise object */
  if ((fp = malloc(sizeof(struct foo))) != NULL) {
    fp->f_count = 1;
    fp->f_id = id;
    /* Initialise object mutex; return NULL if unsuccessful */
    if (pthread_mutex_init(&fp->f_lock, NULL) != 0) {
      free(fp);
      return (NULL);
    }
    idx = HASH(id); /* hash list element index */
    pthread_mutex_lock(&hashlock);
    fp->f_next = fh[idx];
    fh[idx] = fp;
    pthread_mutex_lock(&fp->f_lock);
    pthread_mutex_unlock(&hashlock);
    /* ... continue initialisation of remaining object members ... */
    pthread_mutex_unlock(&fp->f_lock);
  }
  return (fp);
}

/*
 * Increase object reference count.  This function will block.  Before using the
 * object, threads are expected to call this function to add a reference to the
 * object.
 */
void foo_hold(struct foo *fp) {
  pthread_mutex_lock(&hashlock);
  fp->f_count++;
  pthread_mutex_unlock(&hashlock);
}

/* Find an existing object */
struct foo *find_foo(int id) {
  struct foo *fp;

  pthread_mutex_lock(&hashlock);
  for (fp = fh[HASH(id)]; fp != NULL; fp = fp->f_next) {
    if (fp->f_id == id) {
      fp->f_count++;
      break;
    }
  }
  pthread_mutex_unlock(&hashlock);
  return (fp);
}

/*
 * Decrease reference count and release object memory if possible.  This
 * function will block.
 */
void foo_rele(struct foo *fp) {
  struct foo *tfp;
  int idx; /* hash list element index */

  pthread_mutex_lock(&hashlock);
  if (--fp->f_count == 0) { /* last reference; remove from hash list */
    idx = HASH(fp->f_id);
    tfp = fh[idx];
    if (tfp == fp) {
      fh[idx] = fp->f_next;
    } else {
      while (tfp->f_next != fp) {
        tfp = tfp->f_next;
      }
      tfp->f_next = fp->f_next;
    }
    pthread_mutex_unlock(&hashlock);
    pthread_mutex_destroy(&fp->f_lock);
    free(fp);
  } else {
    pthread_mutex_unlock(&hashlock);
  }
}