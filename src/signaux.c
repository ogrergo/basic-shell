#include <signal.h>
#include <errno.h>
#include <stdio.h>

int main() {
  void pri(int sig) {
    printf("intercept sig %d\n", sig);
  }

  struct sigaction sa;
  sa.sa_handler = &pri;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  //sigaction(SIGINT, &sa, 0);
  sigaction(SIGQUIT, &sa, 0);
  sigaction(SIGTSTP, &sa, 0);
  sigaction(SIGTTOU, &sa, 0);
  sigaction(SIGCHLD, &sa, 0);
  sigaction(SIGTTIN, &sa, 0);

  while(1);
}
