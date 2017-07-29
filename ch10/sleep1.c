/*
 * This program uses alarm() and pause() to put a process to sleep for a
 * specified amount of time.  It resembles early implementations of sleep(),
 * but without the problems described in the comments in the function body 
 * corrected.
 */
#include <signal.h>
#include <unistd.h>

static void sig_alrm(int signo) {
  /* nothing to do, just return to wake up the pause() */
}

unsigned int sleep1(unsigned int seconds) {
  /*
   * This function modifies the disposition for SIGALRM, without first saving
   * the current disposition (so that it can be restored before this function
   * returns).
   */
  if (signal(SIGALRM, sig_alrm) == SIG_ERR) {
    return (seconds);
  }
  /*
   * If the caller already has an alarm set, that alrm is erased by this call
   * to alarm().  This can be corrected by looking at the return value of
   * alarm().  If the number of seconds until some previously set alarm is less
   * than the argument, then we should wait only until the existing alarm
   * expires.  If the previously set alam will go off after ours, then before
   * returning, we should reset this alarm to occur at its designated time in
   * the future.
   */
  alarm(seconds);    /* start the timer */

  /*
   * There is arace condition between the call to alarm() aboe and the call to
   * pause() below.  On a busy system, it's possible for the alarm to go off
   * and the signal handler to be called before we call pause() here below.  If
   * that happens, the caller is suspended forever in the call to pause(); 
   * assuming that some other signal isn't caught.
   */
  
  pause();           /* next caught signal wakes up the process */
  return (alarm(0)); /* turn off timer, return unselpt time */
}
