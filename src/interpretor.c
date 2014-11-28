#include "interpretor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <wordexp.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
#define MAX_NAME 40


typedef struct processus {
  pid_t pid;
  pid_t pgid;
  char name[MAX_NAME];

  char end;

  struct processus *next;
} process_t;

typedef struct job {
  pid_t pgid;
  process_t *first;



  int background;
  int stdin;
  int stdout;

  struct job * next;
} job_t;

job_t * jobs_list = NULL;

int shell_fd;
pid_t shell_pgid;
struct termios shell_tmodes;

job_t * get_job(int pgid) {
  if(jobs_list == NULL) {
    printf("job list vide\n");
    return NULL;
  }

  job_t *j_c = jobs_list;
  while(j_c != NULL) {
    printf("process %d\n", j_c->pgid);
    if(j_c->pgid == pgid)
      return j_c;
    j_c = j_c->next;
  }


  return NULL;
}

process_t * get_process(int pid) {
  if(jobs_list == NULL){
    printf("jobs_list null\n");
    return NULL;
  }

  job_t *j_c = jobs_list;
  while(j_c != NULL) {
    process_t * p_c = j_c->first;
    while(p_c != NULL) {
      if(p_c->pid == pid)
        return p_c;
      p_c = p_c->next;
    }
    j_c = j_c->next;
  }
  return NULL;
}

void delete_proc(process_t * proc) {
  free(proc);
}

void delete_job(job_t * job) {
  process_t * to_free = job->first;
  process_t * last;
  while(to_free->next != NULL) {
    last = to_free;
    to_free = to_free->next;
    delete_proc(last);
  }
  free(job);
}

void remove_job(int pgid) {
  if(jobs_list == NULL)
    return;

  job_t *j_c = jobs_list;
  job_t **j_p = &jobs_list;
  while(j_c != NULL) {
    if(j_c->pgid == pgid) {
      *j_p = j_c->next;
      delete_job(j_c);
      return;
    }
    j_p = &(j_c->next);
    j_c = j_c->next;
  }
}

int is_terminated(job_t * job) {
  process_t * p_c = job->first;
  while(p_c != NULL) {
    if(p_c->end == 0) {
      return 0;
    }
    p_c = p_c->next;
  }
  return 1;
}

process_t * new_process() {
  process_t * proc = (process_t*)malloc(sizeof(process_t));
  assert(proc);
  proc->next = NULL;
  return proc;
}

job_t * add_job() {
  job_t * job = (job_t* )malloc(sizeof(job_t));
  assert(job);
  job->first = NULL;

  job->next = jobs_list;
  jobs_list = job;

  return job;
}

void add_process(job_t * job, process_t * proc) {
  proc->next = job->first;
  job->first = proc;
}

void set_process_terminated(job_t * job, pid_t pid) {
  process_t * c_p = job->first;
  while(c_p != NULL) {
    if(c_p->pid == pid) {
      c_p->end = 1;
    }
    c_p = c_p->next;
  }
}

void handle_sigchld(int sig) {
  int pid;
  while ((pid = waitpid((pid_t)(-1), 0, WNOHANG)) > 0) {
    process_t * proc = get_process(pid);
    job_t * job = get_job(proc->pgid);
    set_process_terminated(job, pid);
    if(is_terminated(job)) {
      remove_job(proc->pgid);
    }
  }
}

int init_handler() {
  struct sigaction sa;
  sa.sa_handler = &handle_sigchld;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  if (sigaction(SIGCHLD, &sa, 0) == -1) {
    perror(0);
    return -1;
  }
  return 0;
}

int init() {
  shell_fd = STDIN_FILENO;
  shell_pgid = getpgid(getpid());
  tcgetattr (shell_fd, &shell_tmodes);
  //signal (SIGINT, SIG_IGN);
  //signal (SIGQUIT, SIG_IGN);
  signal (SIGTSTP, SIG_IGN);
  signal (SIGTTIN, SIG_IGN);
  signal (SIGTTOU, SIG_IGN);
  signal (SIGCHLD, SIG_IGN);

  if(!isatty(shell_fd))
    return -1;

  return init_handler();
}

void jobs() {
  printf("|  pid  - name\n");
  job_t * item = jobs_list;
  while(item != NULL) {
    if(item->first != NULL)
      printf("|%6d - %s \n", item->pgid, item->first->name);
      item = item->next;
  }
  exit(0);
}

static void _exec(const char *name, char * const argv[]) {
  if(!strcmp(name, "jobs"))
    jobs();
  else
    execvp(name, argv);
}

static pid_t exec_cmd(char **cmd, pid_t pgid, int pipe_r, int pipe_w, int backgroud) {

  if(setpgid(getpid(), pgid)) {
    perror("setpgid");
    exit(1);
  }

    if(!backgroud) {
      if(tcsetpgrp(shell_fd, pgid)) {
        perror("tcsetpgrp");
        exit(1);
      }
    }


      signal (SIGINT, SIG_DFL);
      signal (SIGQUIT, SIG_DFL);
      signal (SIGTSTP, SIG_DFL);
      signal (SIGTTOU, SIG_DFL);
      signal (SIGCHLD, SIG_DFL);
      signal (SIGTTIN, SIG_DFL);




  if(pipe_r != STDIN_FILENO) {
    dup2(pipe_r, STDIN_FILENO);
    close(pipe_r);
  }

  if(pipe_w != STDOUT_FILENO) {
    dup2(pipe_w, STDOUT_FILENO);
    close(pipe_w);
  }
  // else
    // printf("%d out stdout\n", getpid());




  _exec(cmd[0], cmd);
  perror("exec");
  exit(-1);
}

char ** expansion(char **seq) {
  wordexp_t tab = {.we_wordc = 0, .we_wordv = NULL};
  char *cur = seq[0];
  int i = 0;
  while(cur != NULL) {
    int status = wordexp(cur, &tab, WRDE_APPEND | WRDE_NOCMD | WRDE_UNDEF);
    if(status != 0) {
      printf("substitution error\n");
    }
    i++;
    cur = seq[i];
  }

  char ** resultat = malloc(sizeof(char*) * (tab.we_wordc + 1));
  for(i = 0; i < tab.we_wordc; i++) {
    resultat[i] = malloc(sizeof(char) * strlen(tab.we_wordv[i]));
    strncpy(resultat[i], tab.we_wordv[i], strlen(tab.we_wordv[i]) + 1);
  }
  resultat[tab.we_wordc] = NULL;

  wordfree(&tab);

  return resultat;
}

void free_exp (char ** cmd) {
  for (int i = 0; cmd[i] != NULL; i++)
    free(cmd[i]);

  free(cmd);
}

void interpret_seq(struct cmdline *l) {
  if(l->seq[0] == 0)
    return ;

    job_t * job = add_job();
    job->background = l->bg;
    job->pgid = 0;

  if (l->in) {
    // printf("in: %s\n", l->in);
    job->stdin = open(l->in, O_RDONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
  } else {
    job->stdin = STDIN_FILENO;
  }

  if (l->out) {
    job->stdout = open(l->out, O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
  } else {
    job->stdout = STDOUT_FILENO;
  }

  int i = 0;
  int fd_in = job->stdin;
  int fd_out;
  int fd_pipe[2];



  while(l->seq[i] != NULL) {

    if(l->seq[i + 1]) {
      if(pipe(fd_pipe) < 0) {
        perror("pipe");
        exit(1);
      }
      fd_out = fd_pipe[1];
    } else {
      fd_out = job->stdout;
    }


    char ** cmd = expansion(l->seq[i]);
    
    pid_t pid = fork();
    if(pid < 0) {
      perror("fork");
      return;
    } else if(pid == 0) {

      if(job->pgid == 0)
        job->pgid = getpid();

      if(fd_out != job->stdout)
        close(fd_pipe[0]);

      exec_cmd(cmd, job->pgid, fd_in, fd_out, l->bg);

    } else {
      if(job->pgid == 0) {
        job->pgid = pid;
      }
      setpgid(pid, job->pgid);

      process_t * proc = new_process();

      proc->pid = pid;
      proc->pgid = job->pgid;
      proc->end = 0;
      strncpy(proc->name, cmd[0], MAX_NAME);
      add_process(job, proc);

    }
    free_exp(cmd);

    if(fd_in != STDIN_FILENO)
      close(fd_in);

    if(fd_out != STDOUT_FILENO)
      close(fd_out);

    fd_in = fd_pipe[0];

    i++;
  }

  if(!l->bg) {
   tcsetpgrp (shell_fd, job->pgid);

   pid_t pid;
   while(!is_terminated(job) && (pid = waitpid(-job->pgid, 0, 0)) != -1) {
     set_process_terminated(job, pid);
  }

  remove_job(job->pgid);
  tcsetpgrp(shell_fd, shell_pgid);
  tcsetattr(shell_fd, TCSADRAIN, &shell_tmodes);
 }
}
