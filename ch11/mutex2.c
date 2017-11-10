/*
 * This program extends mutex1 to demonstrate the use of two mutexes.  Deadlocks
 * are avoided by ensuring that when the need arises to acquire two mutexes at
 * the same time, they are always locked in the same order.  The second mutex
 * protects a hash list that is used to keep track of the foo data structures.
 * The hashlock mutex protects both the fh hash table and the f_next hash link
 * field in the foo struct.  The f_lock mutex in the foo struct protects access
 * to the remainder of the foo structure's fields.
 */
#include <pthread.h>
#include <stdlib.h>

#define NHASH 29
#define HASH(id) (((unsigned long)id) % NHASH)

/* Hash list used to store the foo data structs */
struct foo *fh[NHASH];

/* Mutex used to protect the hash list and the f_next hash link field */
pthread_mutex_t hashlock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Sample thread safe struct.  Only thread safe when manipulated by the
 * functions in this file.
 */
struct foo {
  int f_count;            /* object reference counter */
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
  pthread_mutex_lock(&fp->f_lock);
  fp->f_count++;
  pthread_mutex_unlock(&fp->f_lock);
}

/* Find an existing object */
struct foo *find_foo(int id) {
  struct foo *fp;

  pthread_mutex_lock(&hashlock);
  for (fp = fh[HASH(id)]; fp != NULL; fp = fp->f_next) {
    if (fp->f_id == id) {
      foo_hold(fp);
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

  pthread_mutex_lock(&fp->f_lock);
  if (fp->f_count == 1) { /* last reference */
    pthread_mutex_unlock(&fp->f_lock);
    pthread_mutex_lock(&hashlock);
    pthread_mutex_lock(&fp->f_lock);
    /*
     * Need to recheck the condition in case thread was blocked since the last
     * time it had the strucutre mutex and another thread added a reference to
     * the object.  In that case, just need to release this threads reference.
     */
    if (fp->f_count != 1) {
      fp->f_count--;
      pthread_mutex_unlock(&fp->f_lock);
      pthread_mutex_unlock(&hashlock);
      return;
    }
    /* Remove from hash list */
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
    pthread_mutex_unlock(&fp->f_lock);
    pthread_mutex_destroy(&fp->f_lock);
    free(fp);
  } else {
    fp->f_count--;
    pthread_mutex_unlock(&fp->f_lock);
  }
}