#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void safe_close(int fd) {
  if (close(fd) == -1) {
    perror("close error");
    exit(EXIT_FAILURE);
  }
}

void redirect(int old_fd, int new_fd) {
  if (old_fd != new_fd) {
    if (dup2(old_fd, new_fd) != -1) {
      safe_close(old_fd);
    } else {
      perror("dup2 error");
      exit(EXIT_FAILURE);
    }
  }
}

int main(int argc, char ** argv) {
  int i;
  int src = STDIN_FILENO;
  for (i = 1; i < argc - 1; i++) {
    int fd[2];
    if (pipe(fd) < 0) {
      perror("pipe error");
      exit(EXIT_FAILURE);
    }
    int pid = fork();
    if (pid == -1) {
      perror("fork error");
      exit(EXIT_FAILURE);
    } else if (pid == 0) { //child
      safe_close(fd[0]);
      redirect(src, STDIN_FILENO);
      redirect(fd[1], STDOUT_FILENO);
      execlp(argv[i], argv[i], NULL);
    } else { //parent
      safe_close(fd[1]);
      src = fd[0]; //read next command from here
    }
  }
  redirect(src, STDIN_FILENO);
  execlp(argv[i], argv[i], NULL);
  return 0;
}