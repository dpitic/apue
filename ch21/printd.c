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
#define HTTP_SUCCESS(x) ((x) >= 200 && (x) <= 299)

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

  /* Create thread to communicate with the printer */
  err = pthread_create(&tid, NULL, printer_thread, NULL);
  /* Create thread to handle signals */
  if (err == 0) {
    err = pthread_create(&tid, NULL, signal_thread, NULL);
  }
  if (err != 0) {
    log_exit(err, "Can't create thread");
  }
  /*
   * Search the printer spool directory for any pending print jobs.  For each
   * job found on disk, a strucutre is created to let the printer thread know
   * that it should send the file to the printer.
   */
  build_qonstart();

  /* Finished setting up the print spooling daemon */
  log_msg("Daemon initialised");

  /* main thread infinite loop */
  for (;;) {
    /*
     * select() modifies fd set passed to it to include only those fds that
     * satisfy the event, so make a copy of rendezvous set.
     */
    rset = rendezvous;
    /* Wait for one of the file descriptors to become readable */
    if (select(maxfd + 1, &rset, NULL, NULL, NULL) < 0) {
      log_sys("select() failed");
    }
    /* Check rset for a readable file descriptor */
    for (i = 0; i <= maxfd; i++) {
      if (FD_ISSET(i, &rset)) {
        /*
         * Accept the connection and handle the request.
         */
        if ((sockfd = accept(i, NULL, NULL)) < 0) {
          log_ret("accept() failed");
        }
        /* Create thread to handle client connection */
        pthread_create(&tid, NULL, client_thread, (void *)((long)sockfd));
      }
    }
  }
  /* main thread should never reach this exit statement */
  exit(1);
} /* main() */

/**
 * @brief      Initialise the job ID file.
 *
 *             Use a record lock to prevent more than one printer daemon from
 *             running at a time.
 */
void init_request(void) {
  int n;
  char name[FILENMSZ];

  sprintf(name, "%s/%s", SPOOLDIR, JOBFILE);
  jobfd = open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  /*
   * Obtain write lock on job file to prevent other instances of the print spool
   * daemon running.
   */
  if (write_lock(jobfd, 0, SEEK_SET, 0) < 0) {
    log_quit("Daemon already running");
  }

  /*
   * Reuse the name buffer for the job counter.  Job file contains an ASCII
   * integer string representing the next job number.  If the file was newly
   * created, it will be empty.
   */
  if ((n = read(jobfd, name, FILENMSZ)) < 0) {
    log_sys("Can't read job file");
  }
  if (n == 0) { /* New file is empty; set nextjob number to 1 */
    nextjob = 1;
  } else { /* Existing file; get job number (from reused buffer) */
    nextjob = atol(name);
  }
} /* init_request*() */

/**
 * @brief      Initialise printer information from configuration file.
 *
 *             This function is used to set the printer name and address.
 */
void init_printer(void) {
  printer = get_printaddr();
  if (printer == NULL) {
    exit(1); /* message already logged */
  }
  printer_name = printer->ai_canonname;
  if (printer_name == NULL) {
    /* Printer name not defined; use some default name */
    printer_name = "printer";
  }
  log_msg("printer is %s", printer_name);
} /* init_printer() */

/**
 * @brief      Update the job ID file with the next job number.
 * @details    This function is used to write the next job number to the job
 *             file. It does not handle wrap-around of job number.
 */
void update_jobno(void) {
  char buf[32];

  if (lseek(jobfd, 0, SEEK_SET) == -1) {
    log_sys("Can't seek in job file");
  }
  sprintf(buf, "%d", nextjob);
  if (write(jobfd, buf, strlen(buf)) < 0) {
    log_sys("Can't update job file");
  } /* update_jobno() */
}

/**
 * @brief Get the next job number.
 *
 * @return next job number.
 */
int32_t get_newjobno(void) {
  int32_t jobid;

  pthread_mutex_lock(&joblock);
  jobid = nextjob++;
  /* Handle case where jobid wraps around; restart at 1 */
  if (nextjob <= 0) {
    nextjob = 1;
  }
  pthread_mutex_unlock(&joblock);
  return (jobid);
} /* get_newjobno() */

/**
 * @brief      Add a new job to the list of pending jobs.
 *
 *             This function is used to add a new job to the list of pending
 *             jobs and then signal the printer thread that a job is pending.
 *
 * @param      reqp   pointer to printreq structure from client.
 * @param      jobid  print job number.
 */
void add_job(struct printreq *reqp, int32_t jobid) {
  struct job *jp;

  if ((jp = malloc(sizeof(struct job))) == NULL) {
    log_sys("malloc() failed");
  }
  /* Copy request struct from the client into the job structure */
  memcpy(&jp->req, reqp, sizeof(struct printreq));
  jp->jobid = jobid;
  jp->next = NULL;
  pthread_mutex_lock(&joblock);
  jp->prev = jobtail;    /* set new struct prev ptr to last job on list */
  if (jobtail == NULL) { /* List is empty */
    jobhead = jp;        /* job head ptr; this job is at the head of the list */
  } else {               /* List not empty */
    jobtail->next = jp;  /* next ptr of last entry points to this job */
  }
  jobtail = jp; /* job tail ptr; this job is at the end of the list */
  pthread_mutex_unlock(&joblock);
  pthread_cond_signal(&jobwait); /* signal print thread another job avail. */
} /* add_job() */

/**
 * @brief      Replace a job back on the head of the list.
 * @details    This function is used to insert a job at the head of the pending
 *             job list.
 *
 * @param      jp    pointer to job that is to be inserted on the head of the
 *                   list.
 */
void replace_job(struct job *jp) {
  pthread_mutex_lock(&joblock);
  jp->prev = NULL;       /* this job is at the head of the job list */
  jp->next = jobhead;    /* insert this job at the head of the list */
  if (jobhead == NULL) { /* empty list */
    jobtail = jp;        /* this is the only job in the list */
  } else {               /* job list not empty */
    jobhead->prev = jp;  /* insert this job at the head of the list */
  }
  jobhead = jp; /* insert this job at the head of the job list */
  pthread_mutex_unlock(&joblock);
} /* replace_job() */

/**
 * @brief      Remove a job from the list of pending jobs.
 * @details    This function removes a job from the list of pending jobs given a
 *             pointer to the job to be removed. The caller must hold the job
 *             lock mutex.
 *
 * @param      target  pointer to job that should be removed from job list.
 */
void remove_job(struct job *target) {
  /* Set this target job's previous pointer */
  if (target->next != NULL) { /* target is not the last job in the job list */
    target->next->prev = target->prev;
  } else { /* target job is the last job in the job list */
    jobtail = target->prev;
  }
  /* Set this target job's next pointer */
  if (target->prev != NULL) { /* target is not the first job in the job list */
    target->prev->next = target->next;
  } else { /* target job is the first job in the job list */
    jobhead = target->next;
  }
} /* remove_job() */

/**
 * @brief      Check the spool directory for pending jobs on start-up.
 * @details    When the print spooler daemon starts, it uses this function to
 *             build an in-memory list of print jobs from the disk files stored
 *             in the print spooler printer request directory. If the directory
 *             can't be opened, no print jobs are pending, so the function
 *             returns.
 */
void build_qonstart(void) {
  int fd, err, nr;
  int32_t jobid;
  DIR *dirp;
  struct dirent *entp;
  struct printreq req;
  char dname[FILENMSZ], fname[FILENMSZ];

  sprintf(dname, "%s/%s", SPOOLDIR, REQDIR);
  if ((dirp = opendir(dname)) == NULL) {
    /* No print jobs are pending */
    return;
  }
  /* Read each entry in the directory, one at a time; skip . and .. */
  while ((entp = readdir(dirp)) != NULL) {
    /*
     * Skip "." and ".."
     */
    if (strcmp(entp->d_name, ".") == 0 || strcmp(entp->d_name, "..") == 0) {
      continue;
    }
    /*
     * Read the request structure.
     */
    /* Build full pathname of the print request file and open for reading */
    sprintf(fname, "%s/%s/%s", SPOOLDIR, REQDIR, entp->d_name);
    if ((fd = open(fname, O_RDONLY)) < 0) {
      /* Opening file failed; just skip the file */
      continue;
    }
    /* Read the print request structure from the file */
    nr = read(fd, &req, sizeof(struct printreq));
    /* Check to ensure whole strucutre was read */
    if (nr != sizeof(struct printreq)) {
      if (nr < 0) {
        err = errno;
      } else {
        err = EIO;
      }
      close(fd);
      log_msg("build_qonstart(): can't read %s: %s", fname, strerror(err));
      unlink(fname);
      /* Build full pathname of corresponding data file */
      sprintf(fname, "%s/%s/%s", SPOOLDIR, DATADIR, entp->d_name);
      unlink(fname);
      continue;
    }
    /* Able to read the complete print request strucutre */
    jobid = atol(entp->d_name); /* get print job ID */
    log_msg("Adding job %d to queue", jobid);
    add_job(&req, jobid); /* Add request to list of pending print jobs */
  }
  /*
   * Finished reading the directory; readdir() will return NULL; close directory
   * and return.
   */
  closedir(dirp);
} /* build_qonstart() */

/**
 * @brief      Accept a print job from a client.
 * @details    Client thread is spawned from the main thread when a connect
 *             request is accepted. Its job is to receive the file to be
 *             printed from the client print command. A separate thread is
 *             created for each client print request.
 *
 * @param      arg   socket file descriptor.
 */
void *client_thread(void *arg) {
  int n, fd, sockfd, nr, nw, first;
  int32_t jobid;
  pthread_t tid;
  struct printreq req;
  struct printresp res;
  char name[FILENMSZ];
  char buf[IOBUFSZ];

  tid = pthread_self();
  /* Install thread cleanup handler */
  pthread_cleanup_push(client_cleanup, (void *)((long)tid));
  sockfd = (long)arg;
  /* Create worker thread structure & add it to list of active client threads */
  add_worker(tid, sockfd);

  /*
   * Read the request header.
   */
  if ((n = treadn(sockfd, &req, sizeof(struct printreq), 10)) !=
      sizeof(struct printreq)) {
    res.jobid = 0;
    if (n < 0) {
      res.retcode = htonl(errno);
    } else {
      res.retcode = htonl(EIO);
    }
    strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
    writen(sockfd, &res, sizeof(struct printresp));
    pthread_exit((void *)1);
  }
  req.size = ntohl(req.size);
  req.flags = ntohl(req.flags);

  /*
   * Create the data file.
   */
  jobid = get_newjobno();
  sprintf(name, "%s/%s/%d", SPOOLDIR, DATADIR, jobid);
  fd = creat(name, FILEPERM);
  if (fd < 0) {
    res.jobid = 0;
    res.retcode = htonl(errno);
    log_msg("client_thread(): can't create %s: %s", name,
            strerror(res.retcode));
    strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
    writen(sockfd, &res, sizeof(struct printresp));
    pthread_exit((void *)1);
  }

  /*
   * Read the file and store it in the spool directory.  Try to figure out if
   * the file is a PostScript file or a plain text file.
   */
  first = 1;
  while ((nr = tread(sockfd, buf, IOBUFSZ, 20)) > 0) {
    if (first) {
      first = 0;
      if (strncmp(buf, "%!PS", 4) != 0) {
        /* The file doesn't begin with the pattern %!PS; assume text file */
        req.flags |= PR_TEXT;
      }
    }
    nw = write(fd, buf, nr);
    if (nw != nr) {
      res.jobid = 0;
      if (nw < 0) {
        res.retcode = htonl(errno);
      } else {
        res.retcode = htonl(EIO);
      }
      log_msg("client_thread(): can't write %s: %s", name,
              strerror(res.retcode));
      close(fd);
      strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
      writen(sockfd, &res, sizeof(struct printresp));
      unlink(name);
      pthread_exit((void *)1);
    }
  }
  close(fd);

  /*
   * Create the control file.  Then write the print request information to the
   * control file.
   */
  sprintf(name, "%s/%s/%d", SPOOLDIR, REQDIR, jobid);
  fd = creat(name, FILEPERM);
  if (fd < 0) {
    /*
     * Print request control file creation failed; remove data file & terminate
     * thread.
     */
    res.jobid = 0;
    res.retcode = htonl(errno);
    log_msg("client_thread(): can't create %s: %s", name,
            strerror(res.retcode));
    strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
    writen(sockfd, &res, sizeof(struct printresp));
    sprintf(name, "%s/%s/%d", SPOOLDIR, DATADIR, jobid);
    unlink(name);
    pthread_exit((void *)1);
  }

  /* Write print request structure to the control file. */
  nw = write(fd, &req, sizeof(struct printreq));
  if (nw != sizeof(struct printreq)) {
    res.jobid = 0;
    if (nw < 0) {
      res.retcode = htonl(errno);
    } else {
      res.retcode = htonl(EIO);
    }
    log_msg("client_thread(): can't write %s: %s", name, strerror(res.retcode));
    close(fd);
    strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
    writen(sockfd, &res, sizeof(struct printresp));
    unlink(name);
    sprintf(name, "%s/%s/%d", SPOOLDIR, DATADIR, jobid);
    unlink(name);
    pthread_exit((void *)1);
  }
  /*
   * Close file descriptor for control file.  File descriptors are not
   * automatically closed when a thread ends if other threads exist in the
   * process.
   */
  close(fd);

  /*
   * Send response back to client.
   */
  res.retcode = 0; /* successful status */
  res.jobid = htonl(jobid);
  sprintf(res.msg, "Request ID %d", jobid);
  writen(sockfd, &res, sizeof(struct printresp));

  /*
   * Notify the printer thread, clean up, and exit.
   */
  log_msg("Adding job %d to queue", jobid);
  add_job(&req, jobid); /* add job to list of pending print jobs */
  pthread_cleanup_pop(1);
  return ((void *)0);
} /* client_thread() */

/**
 * @brief      Add a worker to the list of worker threads.
 * @details    This function adds a worker thread to the list of active threads.
 *
 * @param      tid     thread ID to add to list of active threads.
 * @param      sockfd  socket file descriptor.
 */
void add_worker(pthread_t tid, int sockfd) {
  struct worker_thread *wtp;

  if ((wtp = malloc(sizeof(struct worker_thread))) == NULL) {
    log_ret("add_worker(): can't malloc()");
    pthread_exit((void *)1);
  }
  wtp->tid = tid;
  wtp->sockfd = sockfd;
  pthread_mutex_lock(&workerlock);
  /* Add worker thread to head of list */
  wtp->prev = NULL;
  wtp->next = workers;
  if (workers == NULL) { /* List is empty */
    workers = wtp;
  } else { /* List is not empty; add to head of list */
    workers->prev = wtp;
  }
  pthread_mutex_unlock(&workerlock);
} /* add_worker() */

/**
 * @brief      Cancel (kill) all outstanding workers.
 * @details    This function walks the list of worker threads and cancels each
 *             one. The function holds the worker lock mutex while walking the
 *             list.
 */
void kill_workers(void) {
  struct worker_thread *wtp;

  pthread_mutex_lock(&workerlock);
  for (wtp = workers; wtp != NULL; wtp = wtp->next) {
    pthread_cancel(wtp->tid);
  }
  pthread_mutex_unlock(&workerlock);
} /* kill_workers() */

/**
 * @brief      Cancellation routine for the worker thread.
 * @details    This is the thread cleanup handler for worker threads that
 *             communicate with the client commands. It is called the thread
 *             calls pthread_exit(), pthread_cleanup_pop() with a nonzero
 *             argument, or responds to a cancellation request. The function
 *             locks the worker lock mutex and searches the list of worker
 *             threads until it finds a matching thread ID. When a match is
 *             found, the worker thread strucutre is removed from the list, and
 *             the search is terminated.
 *
 * @param      arg   Thread ID of the thread terminating.
 */
void client_cleanup(void *arg) {
  struct worker_thread *wtp;
  pthread_t tid;

  tid = (pthread_t)((long)arg);
  pthread_mutex_lock(&workerlock);
  for (wtp = workers; wtp != NULL; wtp = wtp->next) {
    if (wtp->tid == tid) {
      if (wtp->next != NULL) {
        wtp->next->prev = wtp->prev;
      }
      if (wtp->prev != NULL) {
        wtp->prev->next = wtp->next;
      } else {
        workers = wtp->next;
      }
      break;
    }
  }
  pthread_mutex_unlock(&workerlock);
  if (wtp != NULL) {
    /* Close socket file descriptor used by thread to communicate with client */
    close(wtp->sockfd);
    free(wtp);
  }
} /* client_cleanup() */

/**
 * @brief      Deal with signals.
 * @details    This function is run by the thread that is responsible for
 *             handling signals. In the main() function, the signal mask is
 *             initialised to include SIGHUP and SIGTERM. This function calls
 *             sigwait() to wait for one of those signals to occur.
 *
 * @param      arg   Not used; requried for function definition.
 */
void *signal_thread(void *arg) {
  int err, signo;

  for (;;) {
    err = sigwait(&mask, &signo);
    if (err != 0) {
      log_quit("sigwait() failed: %s", strerror(err));
    }
    switch (signo) {
    case SIGHUP:
      /*
       * Schedule to re-read the configuration file.
       */
      pthread_mutex_lock(&configlock);
      reread = 1;
      pthread_mutex_unlock(&configlock);
      break;
    case SIGTERM:
      kill_workers();
      log_msg("Terminate with signal %s", strsignal(signo));
      exit(0);
    default:
      kill_workers();
      log_quit("Unexpected signal %d", signo);
    }
  }
} /* signal_thread() */

/**
 * @brief      Add an option to the IPP header.
 * @details    The format of an attribute is:
 *    * 1-byte tag describing the type of the attribute.
 *    * length of the attribute name stored in binary as a 2-byte integer.
 *    * name of the attribute.
 *    * size of the attribute value.
 *    * value.
 *
 * @param      cp       character pointer to header.
 * @param[in]  tag      The tag
 * @param      optname  Header attribute name.
 * @param      optval   Header attribute value.
 *
 * @return     The address in the header where the next part of the header
 *             should begin.
 */
char *add_option(char *cp, int tag, char *optname, char *optval) {
  int n;
  union {
    int16_t s;
    char c[2];
  } u;

  *cp++ = tag;
  n = strlen(optname);
  u.s = htons(n);
  *cp++ = u.c[0];
  *cp++ = u.c[1];
  strcpy(cp, optname);
  cp += n;
  n = strlen(optval);
  u.s = htons(n);
  *cp++ = u.c[0];
  *cp++ = u.c[1];
  strcpy(cp, optval);
  return (cp + n);
} /* add_option() */

/**
 * @brief      Single thread to communicate with the printer.
 * @details    This function is run by the thread that communicates with the
 *             network printer
 *
 * @param      arg   No used.
 */
void *printer_thread(void *arg) {
  struct job *jp;
  int hlen, ilen, sockfd, fd, nr, nw, extra;
  char *icp; /** IPP header character pointer */
  char *hcp; /* HTTP header character pointer */
  char *p;
  struct ipp_hdr *hp;
  struct stat sbuf;
  struct iovec iov[2];
  char name[FILENMSZ];
  char hbuf[HBUFSZ]; /* HTTP header buffer */
  char ibuf[IBUFSZ]; /* IPP header buffer */
  char buf[IOBUFSZ];
  char str[64];
  struct timespec ts = {60, 0}; /* 1 minute */

  /*
   * Printer thread infinite loop that waits for jobs to transmit to the
   * printer.
   */
  for (;;) {
    /*
     * Get a job to print.
     */
    pthread_mutex_lock(&joblock); /* lock the job list */
    while (jobhead == NULL) {     /* no pending jobs */
      log_msg("printer_thread(): waiting...");
      pthread_cond_wait(&jobwait, &joblock); /* wait for job to arribe */
    }
    /* Print job arrived */
    remove_job(jp = jobhead);
    log_msg("printer_thread(): picked up job %d", jp->jobid);
    pthread_mutex_unlock(&joblock);
    update_jobno();

    /*
     * Check for a change in the configuration file.
     */
    pthread_mutex_lock(&configlock);
    if (reread) {
      freeaddrinfo(printer);
      printer = NULL;
      printer_name = NULL;
      reread = 0;
      pthread_mutex_unlock(&configlock);
      init_printer();
    } else {
      pthread_mutex_unlock(&configlock);
    }

    /*
     * Send job to printer.
     */
    sprintf(name, "%s/%s/%d", SPOOLDIR, DATADIR, jp->jobid);
    if ((fd = open(name, O_RDONLY)) < 0) {
      log_msg("Job %d cancelled - can't open %s: %s", jp->jobid, name,
              strerror(errno));
      free(jp);
      continue;
    }
    if (fstat(fd, &sbuf) < 0) {
      log_msg("Job %d cancelled - can't fstat %s: %s", jp->jobid, name,
              strerror(errno));
      free(jp);
      close(fd);
      continue;
    }
    /* Open a stream socket connected to the printer */
    if ((sockfd = connect_retry(AF_INET, SOCK_STREAM, 0, printer->ai_addr,
                                printer->ai_addrlen)) < 0) {
      log_msg("Job %d deferred - can't contact printer: %s", jp->jobid,
              strerror(errno));
      goto defer;
    }

    /*
     * Set up the IPP header.
     */
    icp = ibuf;
    hp = (struct ipp_hdr *)icp;
    hp->major_version = 1;
    hp->minor_version = 1;
    /* Convert 2-byte operation ID from host to network byte order */
    hp->operation = htons(OP_PRINT_JOB);
    /* Convert 4-byte job ID from host to network byte order */
    hp->request_id = htonl(jp->jobid);
    icp += offsetof(struct ipp_hdr, attr_group);
    *icp++ = TAG_OPERATION_ATTR;
    /* Required attributes */
    icp = add_option(icp, TAG_CHARSET, "attributes-charset", "utf-8");
    icp = add_option(icp, TAG_NATULANG, "attributes-natural-language", "en-us");
    sprintf(str, "http://%s/ipp", printer_name);
    icp = add_option(icp, TAG_URI, "printer-uri", str);
    /* Recommended attribute */
    icp =
        add_option(icp, TAG_NAMEWOLANG, "requesting-user-name", jp->req.usernm);
    /* Optional attribute */
    icp = add_option(icp, TAG_NAMEWOLANG, "job-name", jp->req.jobnm);
    /* Document format attribute */
    if (jp->req.flags & PR_TEXT) {
      p = "text/plain";
      extra = 1;
    } else {
      p = "application/postscript";
      extra = 0;
    }
    icp = add_option(icp, TAG_MIMETYPE, "document-format", p);
    *icp++ = TAG_END_OF_ATTR;
    ilen = icp - ibuf; /* size of the IPP header */

    /*
     * Set up the HTTP header.
     */
    hcp = hbuf;
    sprintf(hcp, "POST /ipp HTTP/1.1\r\n");
    hcp += strlen(hcp);
    sprintf(hcp, "Content-Length: %ld\r\n", (long)sbuf.st_size + ilen + extra);
    hcp += strlen(hcp);
    strcpy(hcp, "Content-Type: application/ipp\r\n");
    hcp += strlen(hcp);
    sprintf(hcp, "Host: %s:%d\r\n", printer_name, IPP_PORT);
    hcp += strlen(hcp);
    *hcp++ = '\r';
    *hcp++ = '\n';
    hlen = hcp - hbuf; /* size of the HTTP header */

    /*
     * Write the headers first.  Then send the file.
     */
    iov[0].iov_base = hbuf;
    iov[0].iov_len = hlen;
    iov[1].iov_base = ibuf;
    iov[1].iov_len = ilen;
    if (writev(sockfd, iov, 2) != hlen + ilen) {
      log_ret("Can't write to printer");
      goto defer;
    }

    if (jp->req.flags & PR_TEXT) {
      /*
       * Hack: Allow PostScript to be printed as plain text.  Sending a
       * backspace as the first character defeats the printer's ability to
       * autosense the file format, while not showing up in the printout.
       */
      if (write(sockfd, "\b", 1) != 1) {
        log_ret("Can't write to printer");
        goto defer;
      }
    }

    /* Send data to be printed in IOBUFSZ chunks */
    while ((nr = read(fd, buf, IOBUFSZ)) > 0) {
      /* write() can send less than requestd amount of data; use writen() */
      if ((nw = writen(sockfd, buf, nr)) != nr) {
        if (nw < 0) {
          log_ret("Can't write to printer");
        } else {
          log_msg("Short write (%d/%d) to printer", nw, nr);
        }
        goto defer;
      }
    }
    if (nr < 0) {
      log_ret("Can't read %s", name);
      goto defer;
    }

    /*
     * Read the response from the printer.
     */
    if (printer_status(sockfd, jp)) {
      unlink(name);
      sprintf(name, "%s/%s/%d", SPOOLDIR, REQDIR, jp->jobid);
      unlink(name);
      free(jp);
      jp = NULL;
    }
  defer:
    close(fd);
    if (sockfd >= 0) {
      close(sockfd);
    }
    if (jp != NULL) {
      /*
       * On error, jp points to the job strucutre for the job that is trying to
       * be printed.  Place the job back on the head of the pending job list
       * and delay for 1 minute.
       */
      replace_job(jp);
      nanosleep(&ts, NULL);
    }
  }
} /* printer_thread() */

/**
 * @brief      Read data from the printer, possibly increasing the buffer.
 * @details    This function is used to read part of the response message from
 *             the printer.
 *
 * @param[in]  sockfd  Socket file descriptor used to communicate with the
 *                     printer.
 * @param      bpp     Pointer to increased buffer starting address.
 * @param[in]  off     Offset in buffer.
 * @param      bszp    Pointer to size of increased buffer.
 *
 * @return     Returns the offset of the end of data in buffer on success; -1 on
 *             failure.
 */
ssize_t readmore(int sockfd, char **bpp, int off, int *bszp) {
  ssize_t nr;
  char *bp = *bpp;
  int bsz = *bszp;

  if (off >= bsz) { /* At end of buffer; reallocate bigger buffer */
    bsz += IOBUFSZ;
    if ((bp = realloc(*bpp, bsz)) == NULL) {
      log_sys("readmore(): Can't allocate bigger buffer");
    }
    *bszp = bsz; /* new buffer size */
    *bpp = bp;   /* starting address of new buffer */
  }
  /*
   * Read as much as the buffer will hold, starting at end of data already in
   * buffer.  Return new end of data offset in the buffer; -1 on error or if
   * timeout expires.
   */
  if ((nr = tread(sockfd, &bp[off], bsz - off, 1)) > 0) {
    return (off + nr); /* success */
  } else {
    return (-1); /* error or timeout expired */
  }
} /* readmore() */

/**
 * @brief      Read and parse the response from the printer.
 * @details    This function is used to read the printer's response to a print
 *             job request. It is not known how the printer will respond; it may
 *             send a response in multiple messages, send a complete response in
 *             one message, or include intermediate acknowledgement, such as
 *             HTTP 100 Continue messages. This function handles all of these
 *             possibilities.
 *
 * @param[in]  sfd   Socket file descriptor used to communicate with the
 * printer.
 * @param      jp    Pointer to print job structure.
 *
 * @return     1 if the request was successful; 0 otherwise.
 */
int printer_status(int sfd, struct job *jp) {
  int i, success, code, len, found, bufsz, datsz;
  int32_t jobid;
  ssize_t nr;
  char *bp, *cp, *statcode, *reason, *contentlen;
  struct ipp_hdr *hp;

  /*
   * Read the HTTP header followed by the IPP response header.  They can be
   * returned in multiple read attempts.  Use the Content-Length specifier to
   * determine how much to read.
   */
  success = 0; /* initialise with failure code */
  bufsz = IOBUFSZ;
  /* Allocate a buffer used to read response from printer */
  if ((bp = malloc(IOBUFSZ)) == NULL) {
    log_sys("printer_status(): Can't allocate read buffer");
  }

  /* Read from printer; expect response to be available within about 5 sec. */
  while ((nr = tread(sfd, bp, bufsz, 5)) > 0) {
    /*
     * Find the status.  Response starts with "HTTP/x.y", so can skip the first
     * 8 characters, and any white space that starts the message.
     */
    cp = bp + 8;
    datsz = nr;
    while (isspace((int)*cp)) {
      cp++;
    }
    /* Status code should follow next; advance pointer to end of code. */
    statcode = cp;
    while (isdigit((int)*cp)) {
      cp++;
    }
    if (cp == statcode) { /* Bad format; log it and move on */
      log_msg(bp);
    } else {
      /* Convert first nondigit character following status code to null byte */
      *cp++ = '\0';
      /* Reason string should follow next */
      reason = cp;
      /* Search for terminating carriage return or line feed */
      while (*cp != '\r' && *cp != '\n') {
        cp++;
      }
      *cp = '\0'; /* terminate with null byte */
      code = atoi(statcode);
      if (HTTP_INFO(code)) { /* Ignore information messages; read more */
        continue;
      }
      /* Expect to see either success or error message */
      if (!HTTP_SUCCESS(code)) { /* Probable error: log it */
        bp[datsz] = '\0';
        log_msg("Error: %s", reason);
        break;
      }

      /*
       * HTTP request was OK, but still need to check IPP status.  Search for
       * the Content-Length attribute.  HTTP header keywords are not case
       * sensitive, so need to check both lower and uppercase characters.
       */
      i = cp - bp;
      for (;;) {
        while (*cp != 'C' && *cp != 'c' && i < datsz) {
          cp++;
          i++;
        }
        if (i >= datsz) { /* if insufficient buffer size; get more header */
          if ((nr = readmore(sfd, &bp, i, &bufsz)) < 0) {
            goto out;
          } else {
            cp = &bp[i];
            datsz += nr;
          }
        }

        /* Case insensitive comparison */
        if (strncasecmp(cp, "Content-Length:", 15) == 0) {
          cp += 15;
          while (isspace((int)*cp)) { /* skip over white spaces */
            cp++;
          }
          /* Read content length attribute value */
          contentlen = cp;
          while (isdigit((int)*cp)) { /* skip over value */
            cp++;
          }
          *cp++ = '\0'; /* Null terminate */
          i = cp - bp;
          len = atoi(contentlen); /* convert numeric string to integer */
          break;
        } else { /* Comparison failed; continue searching byte by byte */
          cp++;
          i++;
        }
      }
      if (i >= datsz) { /* get more header */
        if ((nr = readmore(sfd, &bp, i, &bufsz)) < 0) {
          goto out;
        } else {
          cp = &bp[i];
          datsz += nr;
        }
      }

      /* Search for end of HTTP header (a blank line) */
      found = 0; /* HTTP header found flag */
      while (!found) {
        while (i < datsz - 2) {
          if (*cp == '\n' && *(cp + 1) == '\r' && *(cp + 2) == '\n') {
            found = 1; /* found HTTP header end */
            cp += 3;
            i += 3;
            break;
          }
          cp++;
          i++;
        }
        if (i >= datsz) { /* get more header */
          if ((nr = readmore(sfd, &bp, i, &bufsz)) < 0) {
            goto out;
          } else {
            cp = &bp[i];
            datsz += nr;
          }
        }
      }

      if (datsz - i < len) { /* get more header */
        if ((nr = readmore(sfd, &bp, i, &bufsz)) < 0) {
          goto out;
        } else {
          cp = &bp[i];
          datsz += nr;
        }
      }

      hp = (struct ipp_hdr *)cp;
      i = ntohs(hp->status);         /* convert to host byte order */
      jobid = ntohl(hp->request_id); /* convert to host byte order */

      if (jobid != jp->jobid) {
        /*
         * Different jobs.  Ignore it.
         */
        log_msg("jobid %d status code %d", jobid, i);
        break;
      }

      if (STATCLASS_OK(i)) {
        success = 1;
      }
      break;
    }
  }

out:
  free(bp);
  if (nr < 0) {
    log_msg("jobid %d: error reading printer response: %s", jobid,
            strerror(errno));
  }
  return (success);
} /* printer_status() */
