/*
 * One of the problems using XSI message queues is that you can't use poll() or
 * select() with them, because they aren't associated with file descriptors.
 * However, sockets are associated with file descriptors, and they can be used
 * to notify us when messages arrive.  We'll one thread per message queue.
 * Each thread will block in a call to msgrcv().  When a mesage arrives, the
 * thread will write it down one end of a UNIX domain socket.  Our application
 * will use the other end of the socked to receive the message when poll()
 * indicated data can be read from the socket.
 *
 * This program implements the technique described.  The main() function
 * creates the message queues and UNIX domain sockets, and starts one thread to
 * service each queue.  It uses an infinite loop to poll one of the sockets.
 * When a socket is readable, it reads from the socket and writes the message
 * on the standard output.
 */
#include "apue.h"
#include <poll.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/socket.h>

#define NQ 3       /* number of queues */
#define MAXMSZ 512 /* maximum message size */
#define KEY 0x123  /* key for first message queue */

struct threadinfo {
  int qid;
  int fd;
};

struct mymesg {
  long mtype;
  char mtext[MAXMSZ];
};

void *helper(void *arg) {
  int n;
  struct mymesg m;
  struct threadinfo *tip = arg;

  for (;;) {
    memset(&m, 0, sizeof(m));
    if ((n = msgrcv(tip->qid, &m, MAXMSZ, 0, MSG_NOERROR)) < 0) {
      err_sys("msgrcv() error");
    }
    if (write(tip->fd, m.mtext, n) < 0) {
      err_sys("write() error");
    }
  }
}

int main() {
  int i, n, err;
  int fd[2];
  int qid[NQ];
  struct pollfd pfd[NQ];
  struct threadinfo ti[NQ];
  pthread_t tid[NQ];
  char buf[MAXMSZ];

  for (i = 0; i < NQ; i++) {
    if ((qid[i] = msgget((KEY + i), IPC_CREAT | 0666)) < 0) {
      err_sys("msgget() error");
    }

    printf("queue ID %d is %d\n", i, qid[i]);

    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, fd) < 0) {
      err_sys("socketpair() error");
    }

    pfd[i].fd = fd[0];
    pfd[i].events = POLLIN;
    ti[i].qid = qid[i];
    ti[i].fd = fd[1];
    if ((err = pthread_create(&tid[i], NULL, helper, &ti[i])) != 0) {
      err_exit(err, "pthread_create() error");
    }
  }

  for (;;) {
    if (poll(pfd, NQ, -1) < 0) {
      err_sys("poll() error");
    }
    for (i = 0; i < NQ; i++) {
      if (pfd[i].revents & POLLIN) {
        if ((n = read(pfd[i].fd, buf, sizeof(buf))) < 0) {
          err_sys("read() error");
        }
        buf[n] = 0;
        printf("queue id %d, message %s\n", qid[i], buf);
      }
    }
  }

  exit(0);
}