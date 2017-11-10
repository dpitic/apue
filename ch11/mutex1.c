/*
 * The code sample demonstrates using a mutex to protect a data structure.  When
 * multiple threads need to access a dynamically allocated object, a reference
 * count can be embedded in the object to ensure that it's memory is not freed
 * before all threads have finished using it.  The simple example neglects to
 * address how threads find an object to obtain a reference.  Therefore, even if
 * the reference count is zero, it would be a mistake for a thread to free the
 * object's memory if another thread is blocked on the mutex in a call to 
 * foo_hold().
 */
#include <pthread.h>
#include <stdlib.h>

/* 
 * Sample strucutre designed to be accessed by multiple threads.  This struct is
 * only thread safe when manipulated by the functions in this file.
 */
struct foo {
  int f_count;            /* reference counter */
  pthread_mutex_t f_lock; /* mutex lock */
  int f_id;               /* object id */
  /* ... other struct members here ... */
};

/* 
 * Utility function used to allocate memory and initialise struct foo object. 
 * This includes attempting to initialise the mutex lock in the object.
 * If successful, it returns a pointer to the new object.  On failure, it 
 * returns NULL.
 */
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

/* 
 * Decrease reference count and release object memory if possible.  This
 * function will block.  When a thread is finished with the object, it is
 * expected to call this function to release the reference.  When the last
 * reference is released, the object's memory is freed.  Note that this
 * implementation disregards how threads find the object.  Even if the reference
 * count is zero, it would be a mistake to free the object's memory if another
 * thread is blocked on the mutex in a call to foo_hold().
 */
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
