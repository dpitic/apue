/*
 * The code sample demonstrates using a mutex to protect a data structure.  When
 * multiple threads need to access a dynamically allocated object, a reference
 * count can be embedded in the object to ensure that it's memory is not freed
 * before all threads have finished using it.
 */
#include <pthread.h>
#include <stdlib.h>

/* Sample strucutre accessed by multiple threads */
struct foo {
  int f_count;            /* reference counter */
  pthread_mutex_t f_lock; /* mutex lock */
  int f_id;               /* object id */
  /* ... other struct members here ... */
};

/* Utility function used to allocate and initialise struct foo object */
struct foo *foo_alloc(int id) {
  struct foo *fp;
  /* Allocate memory and initialise structure */
  if ((fp = malloc(sizeof(struct foo))) != NULL) {
    fp->f_count = 1;
    fp->f_id = id;
    if (pthread_mutex_init(&fp->f_lock, NULL) != 0) {
      free(fp);
      return (NULL);
    }
    /* ... continue initialisation of remaining object members ... */
  }
  return (fp);
}

/* Increase object reference count */
void foo_hold(struct foo *fp) {
  pthread_mutex_lock(&fp->f_lock);
  fp->f_count++;
  pthread_mutex_unlock(&fp->f_lock);
}

/* Decrease reference count and release object memory if possible */
void foo_rele(struct foo *fp) {
  pthread_mutex_lock(&fp->f_lock);
  if (--fp->f_count == 0) { /* last reference */
    pthread_mutex_unlock(&fp->f_lock);
    pthread_mutex_destroy(&fp->f_lock);
    free(fp);
  } else {
    /* Another object still holds a reference; so can't release memory yet */
    pthread_mutex_unlock(&fp->f_lock);
  }
}