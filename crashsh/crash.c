#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXLINE 1024
#define MAXJOBS 32

typedef struct {
  int jid;   // job id (up to INT_MAX)
  pid_t pid; // process id
  bool act;  // active?
  char command[MAXLINE];
} Job;

// global jobs list
Job jobs[MAXJOBS];
int nextjid = 1;
int jcount = 0;

pid_t foreground_pid = 0;

Job *find_pid(pid_t pid) {
  for (int i = 0; i < MAXJOBS; ++i) {
    if (jobs[i].pid == pid)
      return &jobs[i];
  }
  return NULL;
}

Job *find_jid(int jid) {
  for (int i = 0; i < MAXJOBS; ++i) {
    if (jobs[i].jid == jid)
      return &jobs[i];
  }
  return NULL;
}

void remove_job(pid_t pid) {
  sigset_t mask, prev_mask; //to restore mask
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  sigprocmask(SIG_BLOCK, &mask, &prev_mask);

  for (int i = 0; i < MAXJOBS; ++i) {
    if (jobs[i].pid == pid) {
      jobs[i].pid = 0;
      jobs[i].act = false;
      jcount--;

      if (foreground_pid == pid) {
        foreground_pid = 0;
      }
      sigprocmask(SIG_SETMASK, &prev_mask, NULL);
      return;
    }
  }
  sigprocmask(SIG_SETMASK, &prev_mask, NULL);
}

void sigchld_handler(int sig) {
  int status;
  pid_t pid;

  while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
    Job *job = find_pid(pid);
    if (job) {
      char msg[MAXLINE];
      int len;

      if (WIFEXITED(status)) {
        len = snprintf(msg, MAXLINE, "[%d] (%d)  finished  %s\n", job->jid,
                       (int)job->pid, job->command);
        write(STDOUT_FILENO, msg, len);
        remove_job(pid);
      } else if (WIFSIGNALED(status)) {
        if (WCOREDUMP(status)) {
          len = snprintf(msg, MAXLINE, "[%d] (%d)  killed (core dumped)  %s\n",
                         job->jid, (int)job->pid, job->command);
          write(STDOUT_FILENO, msg, len);
          remove_job(job->pid);
        } else {
          len = snprintf(msg, MAXLINE, "[%d] (%d)  killed  %s\n", job->jid,
                         (int)job->pid, job->command);
          write(STDOUT_FILENO, msg, len);
          remove_job(job->pid);
        }
      } else if (WIFSTOPPED(status)) {
        len = snprintf(msg, MAXLINE, "[%d] (%d)  suspended  %s\n", job->jid,
                       (int)job->pid, job->command);
        write(STDOUT_FILENO, msg, len);
        job->act = false;

        if (foreground_pid == pid) {
          foreground_pid = 0;
        }
      } else if (WIFCONTINUED(status)) {
        len = snprintf(msg, MAXLINE, "[%d] (%d)  continued  %s\n", job->jid,
                       (int)job->pid, job->command);
        write(STDOUT_FILENO, msg, len);
        job->act = true;
      }
    }
  }
}

void sigint_handler(int sig) {
  if (foreground_pid > 0) {
    kill(foreground_pid, SIGINT);
  }
  // if no fg, ignore sigint
}

void sigtstp_handler(int sig) {
  if (foreground_pid > 0) {
    kill(foreground_pid, SIGTSTP);
  }
  // if no fg, ignore sigtstp
}

void sigquit_handler(int sig) {
  if (foreground_pid > 0) {
    kill(foreground_pid, SIGQUIT);
  } else {
    exit(0);
  }
}

void add_job(pid_t pid, const char *command, bool bg) {
  if (jcount >= MAXJOBS) {
    const char *msg = "ERROR: too many jobs\n";
    write(STDERR_FILENO, msg, strlen(msg));
    return;
  }

  int j = -1;
  for (int i = 0; i < MAXJOBS; ++i) {
    if (jobs[i].pid == 0) {
      j = i;
      break;
    }
  }

  if (j == -1) {
    const char *msg = "ERROR: too many jobs\n";
    write(STDERR_FILENO, msg, strlen(msg));
    return;
  }

  jobs[j].jid = nextjid++;
  jobs[j].pid = pid;
  jobs[j].act = true;
  strncpy(jobs[j].command, command, MAXLINE - 1);
  jobs[j].command[MAXLINE - 1] = '\0';
  jcount++;

  if (bg) {
    char msg[MAXLINE];
    int len = snprintf(msg, MAXLINE, "[%d] (%d)  running  %s\n", jobs[j].jid,
                       (int)pid, jobs[j].command);
    write(STDOUT_FILENO, msg, len);
  } else {
    foreground_pid = pid;
  }
}

void nuke_pid_or_jid(const char *arg) {
  if (arg[0] == '%') {
    char *endptr;
    int jid = strtol(arg + 1, &endptr, 10);

    if (*endptr != '\0' || endptr == arg + 1) {
      char error[MAXLINE];
      snprintf(error, MAXLINE, "ERROR: bad argument for nuke: %s\n", arg);
      write(STDERR_FILENO, error, strlen(error));
      return;
    }

    if (jid <= 0) {
      char error[MAXLINE];
      snprintf(error, MAXLINE, "ERROR: bad argument for nuke: %s\n", arg);
      write(STDERR_FILENO, error, strlen(error));
      return;
    }

    Job *job = find_jid(jid);
    if (job != NULL && job->pid) {
      kill(job->pid, SIGKILL);
    } else {
      char error[MAXLINE];
      snprintf(error, MAXLINE, "ERROR: no job %d\n", jid);
      write(STDERR_FILENO, error, strlen(error));
      return;
    }

  } else {
    char *endptr;
    pid_t pid = strtol(arg, &endptr, 10);

    if (*endptr != '\0' || endptr == arg + 1) {
      char error[MAXLINE];
      snprintf(error, MAXLINE, "ERROR: bad argument for nuke: %s\n", arg);
      write(STDERR_FILENO, error, strlen(error));
      return;
    }

    Job *job = find_pid(pid);
    if (job != NULL && job->pid) {
      kill(job->pid, SIGKILL);
    } else {
      char error[MAXLINE];
      snprintf(error, MAXLINE, "ERROR: no PID %d\n", pid);
      write(STDERR_FILENO, error, strlen(error));
      return;
    }
  }
}

void nuke(const char **toks) {
  if (toks[1] == NULL) {
    for (int i = 0; i < MAXJOBS; ++i) {
      if (jobs[i].pid) {
        kill(jobs[i].pid, SIGKILL);
      }
    }
    return;
  }

  for (int i = 1; toks[i] != NULL; ++i) {
    nuke_pid_or_jid(toks[i]);
  }
}

void bg_job(const char **toks) {
  if (toks[1] == NULL) {
    const char *msg = "ERROR: bg needs some arguments\n";
    write(STDERR_FILENO, msg, strlen(msg));
    return;
  }

  for (int i = 1; toks[i] != NULL; ++i) {
    const char *arg = toks[i];
    Job *job = NULL;

    if (arg[0] == '%') {
      char *endptr;
      int jid = strtol(arg + 1, &endptr, 10);

      if (*endptr != '\0' || endptr == arg + 1 || jid <= 0) {
        char error[MAXLINE];
        snprintf(error, MAXLINE, "ERROR: bad argument for bg: %s\n", arg);
        write(STDERR_FILENO, error, strlen(error));
        continue;
      }

      job = find_jid(jid);
      if (job == NULL || job->pid == 0) {
        char error[MAXLINE];
        snprintf(error, MAXLINE, "ERROR: no job %d\n", jid);
        write(STDERR_FILENO, error, strlen(error));
        continue;
      }
    } else {
      char *endptr;
      pid_t pid = strtol(arg, &endptr, 10);

      if (*endptr != '\0' || endptr == arg || pid <= 0) {
        char error[MAXLINE];
        snprintf(error, MAXLINE, "ERROR: bad argument for bg: %s\n", arg);
        write(STDERR_FILENO, error, strlen(error));
        continue;
      }

      job = find_pid(pid);
      if (job == NULL || job->pid == 0) {
        char error[MAXLINE];
        snprintf(error, MAXLINE, "ERROR: no PID %d\n", pid);
        write(STDERR_FILENO, error, strlen(error));
        continue;
      }
    }

    // Send SIGCONT signal to the process
    kill(job->pid, SIGCONT);
    job->act = true;
    
    char msg[MAXLINE];
    int len = snprintf(msg, MAXLINE, "[%d] (%d)  continued  %s\n", job->jid,
                     (int)job->pid, job->command);
    write(STDOUT_FILENO, msg, len);
  }
}

void fg_job(const char **toks) {
  if (toks[1] == NULL || toks[2] != NULL) {
    const char *msg = "ERROR: fg needs exactly one argument\n";
    write(STDERR_FILENO, msg, strlen(msg));
    return;
  }

  const char *arg = toks[1];
  Job *job = NULL;

  if (arg[0] == '%') {
    char *endptr;
    int jid = strtol(arg + 1, &endptr, 10);

    if (*endptr != '\0' || endptr == arg + 1 || jid <= 0) {
      char error[MAXLINE];
      snprintf(error, MAXLINE, "ERROR: bad argument for fg: %s\n", arg);
      write(STDERR_FILENO, error, strlen(error));
      return;
    }

    job = find_jid(jid);
    if (job == NULL || job->pid == 0) {
      char error[MAXLINE];
      snprintf(error, MAXLINE, "ERROR: no job %d\n", jid);
      write(STDERR_FILENO, error, strlen(error));
      return;
    }
  } else {
    char *endptr;
    pid_t pid = strtol(arg, &endptr, 10);

    if (*endptr != '\0' || endptr == arg || pid <= 0) {
      char error[MAXLINE];
      snprintf(error, MAXLINE, "ERROR: bad argument for fg: %s\n", arg);
      write(STDERR_FILENO, error, strlen(error));
      return;
    }

    job = find_pid(pid);
    if (job == NULL || job->pid == 0) {
      char error[MAXLINE];
      snprintf(error, MAXLINE, "ERROR: no PID %d\n", pid);
      write(STDERR_FILENO, error, strlen(error));
      return;
    }
  }

  foreground_pid = job->pid;

  // If job was stopped, continue it
  if (!job->act) {
    kill(job->pid, SIGCONT);
    
    char msg[MAXLINE];
    int len = snprintf(msg, MAXLINE, "[%d] (%d)  continued  %s\n", job->jid,
                     (int)job->pid, job->command);
    write(STDOUT_FILENO, msg, len);
    job->act = true;
  }

  int status;
  waitpid(job->pid, &status, WUNTRACED);

  if (WIFEXITED(status)) {
    char msg[MAXLINE];
    int len = snprintf(msg, MAXLINE, "[%d] (%d)  finished  %s\n", job->jid,
                     (int)job->pid, job->command);
    write(STDOUT_FILENO, msg, len);
    remove_job(job->pid);
  } else if (WIFSIGNALED(status)) {
    char msg[MAXLINE];
    int len;

    if (WCOREDUMP(status)) {
      len = snprintf(msg, MAXLINE, "[%d] (%d)  killed (core dumped)  %s\n",
                     job->jid, (int)job->pid, job->command);
    } else {
      len = snprintf(msg, MAXLINE, "[%d] (%d)  killed  %s\n", job->jid,
                     (int)job->pid, job->command);
    }
    write(STDOUT_FILENO, msg, len);
    remove_job(job->pid);
  } else if (WIFSTOPPED(status)) {
    char msg[MAXLINE];
    int len = snprintf(msg, MAXLINE, "[%d] (%d)  suspended  %s\n", job->jid,
                     (int)job->pid, job->command);
    write(STDOUT_FILENO, msg, len);
    job->act = false;
    foreground_pid = 0;
  }
}

void eval(const char **toks, bool bg) { // bg is true iff command ended with &
  assert(toks);
  if (*toks == NULL)
    return;
  if (strcmp(toks[0], "quit") == 0) {
    if (toks[1] != NULL) {
      const char *msg = "ERROR: quit takes no arguments\n";
      write(STDERR_FILENO, msg, strlen(msg));
      return;
    } else {
      exit(0);
    }
  }

  // jobs
  if (strcmp(toks[0], "jobs") == 0) {
    if (toks[1] != NULL) {
      const char *msg = "ERROR: jobs takes no arguments\n";
      write(STDERR_FILENO, msg, strlen(msg));
      return;
    }
    for (int i = 0; i < MAXJOBS; ++i) {
      if ((int)jobs[i].pid) {
        char msg[MAXLINE];
        int len;
        if (jobs[i].act) {
          len = snprintf(msg, MAXLINE, "[%d] (%d)  running  %s\n",
                       jobs[i].jid, (int)jobs[i].pid, jobs[i].command);
        } else {
          len = snprintf(msg, MAXLINE, "[%d] (%d)  suspended  %s\n",
                       jobs[i].jid, (int)jobs[i].pid, jobs[i].command);
        }
        write(STDOUT_FILENO, msg, len);
      }
    }
    return;
  }

  if (strcmp(toks[0], "nuke") == 0) {
    nuke(toks);
    return;
  }

  if (strcmp(toks[0], "fg") == 0) {
    fg_job(toks);
    return;
  }
  
  if (strcmp(toks[0], "bg") == 0) {
    bg_job(toks);
    return;
  }

  if (jcount >= MAXJOBS) {
    const char *msg = "ERROR: too many jobs\n";
    write(STDERR_FILENO, msg, strlen(msg));
    return;
  }

  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  sigprocmask(SIG_BLOCK, &mask, NULL);

  pid_t pid = fork();

  if (pid < 0) {
    // fork failed
    char error[MAXLINE];
    snprintf(error, MAXLINE, "ERROR: cannot run %s\n", toks[0]);
    write(STDERR_FILENO, error, strlen(error));
  } else if (pid == 0) {
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    // child
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGCONT, SIG_DFL);
    
    execvp(toks[0], (char *const *)toks);

    // execvp failed
    char error[MAXLINE];
    snprintf(error, MAXLINE, "ERROR: cannot run %s\n", toks[0]);
    write(STDERR_FILENO, error, strlen(error));
    exit(1);
  } else {
    add_job(pid, toks[0], bg);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    if (!bg) {
      int status;
      waitpid(pid, &status, WUNTRACED);
      // if job stopped rather than terminated, let sighanlder deal with it
      // if not, then print the following

      if (WIFEXITED(status)) {
        char msg[MAXLINE];
        Job *j = find_pid(pid);
        if (j) {
          int len = snprintf(msg, MAXLINE, "[%d] (%d)  finished  %s\n",
                           j->jid, (int)pid, toks[0]);
          write(STDOUT_FILENO, msg, len);
          remove_job(pid);
        }
      } else if (WIFSIGNALED(status)) {
        char msg[MAXLINE];
        int len;
        Job *j = find_pid(pid);
        if (j) {
          if (WCOREDUMP(status)) {
            len = snprintf(msg, MAXLINE, "[%d] (%d)  killed (core dumped)  %s\n",
                         j->jid, (int)pid, toks[0]);
          } else {
            len = snprintf(msg, MAXLINE, "[%d] (%d)  killed  %s\n",
                         j->jid, (int)pid, toks[0]);
          }
          write(STDOUT_FILENO, msg, len);
          remove_job(pid);
        }
      } else if (WIFSTOPPED(status)) {
        Job *j = find_pid(pid);
        if (j) {
          char msg[MAXLINE];
          int len = snprintf(msg, MAXLINE, "[%d] (%d)  suspended  %s\n",
                           j->jid, (int)pid, toks[0]);
          write(STDOUT_FILENO, msg, len);
          j->act = false;
          foreground_pid = 0;
        }
      }
    }
  }
}

void parse_and_eval(char *s) {
  assert(s);
  const char *toks[MAXLINE + 1];

  while (*s != '\0') {
    bool end = false;
    bool bg = false;
    int t = 0;

    while (*s != '\0' && !end) {
      while (*s == '\n' || *s == '\t' || *s == ' ')
        ++s;
      if (*s != ';' && *s != '&' && *s != '\0')
        toks[t++] = s;
      while (strchr("&;\n\t ", *s) == NULL)
        ++s;
      switch (*s) {
      case '&':
        bg = true;
        end = true;
        break;
      case ';':
        end = true;
        break;
      }
      if (*s)
        *s++ = '\0';
    }
    toks[t] = NULL;
    eval(toks, bg);
  }
}

void prompt() {
  const char *prompt = "crash> ";
  ssize_t nbytes = write(STDOUT_FILENO, prompt, strlen(prompt));
}

int repl() {
  char *buf = NULL;
  size_t len = 0;
  ssize_t nread;

  while (1) {
    prompt();
    nread = getline(&buf, &len, stdin);

    if (nread == -1) {
      if (feof(stdin)) {
        exit(0); // for ctrl+d
      } else {
        perror("ERROR");
        if (buf)
          free(buf);
        return 1;
      }
    }
    parse_and_eval(buf);
  }

  if (buf != NULL)
    free(buf);
  return 0;
}

int main(int argc, char **argv) {
  for (int i = 0; i < MAXJOBS; ++i) {
    jobs[i].pid = 0;
  }

  struct sigaction sa;
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART; // for syscalls interrupted by sigchld, restarted
                            // instead of fail
  if (sigaction(SIGCHLD, &sa, NULL) < 0) {
    perror("sigaction error");
    exit(1);
  }

  struct sigaction sa_int;
  sa_int.sa_handler = sigint_handler;
  sigemptyset(&sa_int.sa_mask);
  sa_int.sa_flags = SA_RESTART;
  if (sigaction(SIGINT, &sa_int, NULL) < 0) {
    perror("sigaction error");
    exit(1);
  }

  struct sigaction sa_tstp;
  sa_tstp.sa_handler = sigtstp_handler;
  sigemptyset(&sa_tstp.sa_mask);
  sa_tstp.sa_flags = SA_RESTART;
  if (sigaction(SIGTSTP, &sa_tstp, NULL) < 0) {
    perror("sigaction error");
    exit(1);
  }

  struct sigaction sa_quit;
  sa_quit.sa_handler = sigquit_handler;
  sigemptyset(&sa_quit.sa_mask);
  sa_quit.sa_flags = SA_RESTART;
  if (sigaction(SIGQUIT, &sa_quit, NULL) < 0) {
    perror("sigaction error");
    exit(1);
  }

  return repl();
}