/*
 * Print server daemon.
 */
#include "apue.h"
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/uio.h>

#include "ipp.h"
#include "print.h"

/*
 * Macros used for the HTTP response from the printer.
 */
#define HTTP_INFO(x) ((x) >= 100 && (x) <= 199)
#define HTTP_SUCCESS(X) ((x) >= 200 && (x) <= 299)

/**
 * Structure used to describe a print job.
 */
struct job {
  struct job *next;    /* next job in list */
  struct job *prev;    /* previous job in list */
  int32_t jobid;       /* print job ID */
  struct printreq req; /* copy of print request */
};

/**
 * Structure used to describe a thread processing a client request.
 */
struct worker_thread {
  struct worker_thread *next; /* next in list */
  struct worker_thread *prev; /* previous in list */
  pthread_t tid;              /* thread ID */
  int sockfd;                 /* socket file descriptor */
};

/**
 * Logging flag: 0 = log messages to system log; 1 = log to stderr.
 */
int log_to_stderr = 0;

/*
 * Printer related variables.
 */
/** Stores the network address of the printer */
struct addrinfo *printer;
/** Host name of the printer */
char *printer_name;
/** Protect access to reread variable */
pthread_mutex_t configlock = PTHREAD_MUTEX_INITIALIZER;
/** Flag to indicate that daemon needs to reread the configuration file */
int reread;

/*
 * Thread related variables.
 */
/** Pointer to head of doubly linked list of threads that receive files from
 * clients. */
struct worker_thread *workers;
/** Protect access to workers list */
pthread_mutex_t workerlock = PTHREAD_MUTEX_INITIALIZER;
/** Signal mask used by the threads */
sigset_t mask;

/*
 * Job related variables.
 */
/** jobhead and jobtail are the start and end of the list of pending jobs */
struct job *jobhead, *jobtail;
/** File descriptor for the job file */
int jobfd;
/** ID of the next print job to be received by the print server. */
int32_t nextjob;
/** Mutex used to protect the linked list of jobs and the condiiton. */
pthread_mutex_t joblock = PTHREAD_MUTEX_INITIALIZER;
/** Condition variable for waiting jobs. */
pthread_cond_t jobwait = PTHREAD_COND_INITIALIZER;

/*
 * Function prototypes.
 */
void init_request(void);
void init_printer(void);
void update_jobno(void);
int32_t get_newjobno(void);
void add_job(struct printreq *, int32_t);
void replace_job(struct job *);
void remove_job(struct job *);
void build_qonstart(void);
void *client_thread(void *);
void *printer_thread(void *);
void *signal_thread(void *);
ssize_t readmore(int, char **, int, int *);
int printer_status(int, struct job *);
void add_worker(pthread_t, int);
void kill_workers(void);
void client_cleanup(void *);

/*
 * Main print server thread.  Accepts connect requests from clients and spawns
 * additional threads to service requests.
 */
int main(int argc, char *argv[]) {
  pthread_t tid;
  struct addrinfo *ailist, *aip;
  int sockfd, err, i, n, maxfd;
  char *host;
  fd_set rendezvous, rset;
  struct sigaction sa;
  struct passwd *pwdp;

  if (argc != 1) {
    err_quit("Usage: %s", argv[0]);
  }
  /*
   * Become daemon; can no longer print error messages to stderr, can only log
   * errors from now on.
   */
  daemonize("printd");

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = SIG_IGN;
  /*
   * Will be writing to socket file descriptors, so don't want a write error to
   * trigger SIGPIPE, because the default action is to kill the process.
   */
  if (sigaction(SIGPIPE, &sa, NULL) < 0) {
    log_sys("sigaction() failed");
  }
  /*
   * Set the signal mask of the thread to include SIGHUP and SIGTERM.  All
   * threads created will inherit this signal mask.  The signals are used to
   * control the daemon as follows:
   *   SIGHUP: tell daemon to reread its configuration file.
   *   SIGTERM: tell daemon to clean up and exit gracefully.
   */
  sigemptyset(&mask);
  sigaddset(&mask, SIGHUP);
  sigaddset(&mask, SIGTERM);
  if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0) {
    log_sys("pthread_sigmask() failed");
  }

  /* Get the maximum size of a host name */
  n = sysconf(_SC_HOST_NAME_MAX);
  if (n < 0) {
    n = HOST_NAME_MAX; /* best guess */
  }
  /* Allocate memory to hold the host name */
  if ((host = malloc(n)) == NULL) {
    log_sys("malloc() error");
  }
  if (gethostname(host, n) < 0) {
    log_sys("gethostname() error");
  }

  /*
   * Try to find the network address of the daemon providing the printer
   * spooling service.
   */
  if ((err = getaddrlist(host, "print", &ailist)) != 0) {
    log_quit("getaddrinfo() error: %s", gai_strerror(err));
    exit(1);
  }

  /* Clear the fd set used to wait for client connect requests */
  FD_ZERO(&rendezvous);
  maxfd = -1; /* ensure first file descriptor allocated is > maxfd */
  /*
   * Call initserver() on each network address that the print service needs to
   * be provide on, to initialise a socket.
   */
  for (aip = ailist; aip != NULL; aip = aip->ai_next) {
    if ((sockfd = initserver(SOCK_STREAM, aip->ai_addr, aip->ai_addrlen,
                             QLEN)) >= 0) {
      /*
       * Add the file descriptor to the fd set used to wait for client connect
       * requests.
       */
      FD_SET(sockfd, &rendezvous);
      if (sockfd > maxfd) {
        maxfd = sockfd;
      }
    }
  }
  if (maxfd == -1) {
    /* Can't enable printer spooling service; log message and quit */
    log_quit("service not enabled");
  }

  /*
   * Daemon needs root privileges to bind a socket to a reserved port number.
   * Now this is done, lower its privileges by changing its user and group ID
   * to the one associated with the LPNAME account.
   */
  pwdp = getpwnam(LPNAME);
  if (pwdp == NULL) {
    log_sys("Can't find user %s", LPNAME);
  }
  if (pwdp->pw_uid == 0) {
    log_quit("User %s is privileged", LPNAME);
  }
  /* Try to change the real and effective IDs to LPNAME account */
  if (setgid(pwdp->pw_gid) < 0 || setuid(pwdp->pw_uid) < 0) {
    log_sys("Can't change IDs to user %s", LPNAME);
  }

  init_request(); /* initialise job requests & ensure only 1 daemon running */
  init_printer(); /* initialise printer information */
}