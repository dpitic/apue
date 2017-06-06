/*
 * This program executes each command-line argument as a shell command string,
 * timing the command and printing the values from the tms structure.
 */
#include "apue.h"
#include <sys/times.h>

static void pr_times(clock_t, struct tms *, struct tms *);
static void do_cmd(char *);

int main(int argc, char *argv[]) {
  int i;

  setbuf(stdout, NULL);
  for (i = 1; i < argc; i++) {
    do_cmd(argv[i]); /* once for each command-line arg */
  }
  exit(0);
}

/* Execute and time the "cmd" */
static void do_cmd(char *cmd) {
  struct tms tms_start, tms_end;
  clock_t start, end;
  int status;

  printf("\ncommand: %s\n", cmd);

  if ((start = times(&tms_start)) == -1) {    /* starting values */
    err_sys("times error");
  }

  if ((status = system(cmd)) < 0) {           /* execute command */
    err_sys("system() error");
  }

  if ((end = times(&tms_end)) == -1) {        /* ending values */
    err_sys("times error");
  }

  pr_times(end - start, &tms_start, &tms_end);
  pr_exit(status);
}

static void pr_times(clock_t real, struct tms *tms_start, struct tms *tms_end) {
  static long clktck = 0;

  if (clktck == 0) {    /* fetch clock ticks per second; first time */
    if ((clktck = sysconf(_SC_CLK_TCK)) < 0) {
      err_sys("sysconf error");
    }
  }

  printf("      real:  %7.2f\n", real / (double)clktck);
  printf("      user:  %7.2f\n",
    (tms_end->tms_utime - tms_start->tms_utime) / (double)clktck);
  printf("       sys:  %7.2f\n",
    (tms_end->tms_stime - tms_start->tms_stime) / (double)clktck);
  printf("child user:  %7.2f\n",
    (tms_end->tms_cutime - tms_start->tms_cutime) / (double)clktck);
  printf(" child sys:  %7.2f\n",
    (tms_end->tms_cstime - tms_start->tms_cstime) / (double)clktck);
}