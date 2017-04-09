/*
 * Extension of the simple shell program that catches the SIGINT signal.
 */
 #include "apue.h"
 #include <sys/wait.h>

 static void sig_int(int);          /* signal handler */

 int main(void)
 {
     char       buf[MAXLINE];       /* input buffer */
     pid_t      pid;
     int        status;             /* termination status of child process */

     if (signal(SIGINT, sig_int) == SIG_ERR) {
         err_sys("signal error");
     }

     printf("%% ");         /* shell prompt */
     while (fgets(buf, MAXLINE, stdin) != NULL) {
         if (buf[strlen(buf) - 1] == '\n') {
             buf[strlen(buf) - 1] = 0;      /* replace newline with null */
         }

         /* create the child process to execute the input command */
         if ((pid = fork()) < 0) {      /* pid of child process */
             err_sys("fork error");
         } else if (pid == 0) {         /* child */
            execlp(buf, buf, (char *)0);    /* execute command in child proc. */
            /* exec() functions return only if an error has occurred */
            err_ret("couldn't execute: %s", buf);
            exit(127);
         }

         /* parent; wait for child to terminate */
         if ((pid = waitpid(pid, &status, 0)) < 0) {
             err_sys("waitpid error");
         }
         printf("%% ");
     }
     exit(0);
 }

 /* 
  * Signal handler implementation. This signal handler simply outputs a string
  * to stdout to indicate that an interrupt was caught and handled.
  */
void sig_int(int signo)
{
    printf("interrupt\n%% ");
}