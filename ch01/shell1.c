/*
 * This is a simple implementation of a shell program. It reads commands from
 * stdin and executes the commands. The limitation of this program is that
 * arguments cannot be passed to the command to be executed.
 */
 #include "apue.h"
 #include <sys/wait.h>

 int main(void)
 {
     char   buf[MAXLINE];       /* input buffer */
     pid_t  pid;
     int    status;             /* termination status of child process */

     printf("%% ");         /* shell prompt */
     while (fgets(buf, MAXLINE, stdin) != NULL) {
         if (buf[strlen(buf) - 1] == '\n') {
             buf[strlen(buf) - 1] = 0;      /* replace newline with null */
         }

         /* create the child process to execute the input command */
         if ((pid = fork()) < 0) {  /* pid of child process */
             err_sys("fork error");
         } else if (pid == 0) {     /* child */
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