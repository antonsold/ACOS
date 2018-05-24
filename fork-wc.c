//
// Created by anton on 21.05.18.
//

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#define L 250

int min(int a, int b) {
  return a < b ? a : b;
}

int main(int argc, char** argv){
  char s[2*L];
  FILE* f = fopen(argv[1], "r");
  if (!f) {
    perror("Can't open file!");
    exit(EXIT_FAILURE);
  }
  int n_threads = atoi(argv[2]);
  int n_files = 0, n_chars = 0, n_words = 0, n_lines = 0;
  while(fscanf(f, "%s", s) != -1) {
    n_files++;
  }
  fseek(f, 0, SEEK_SET);
  char** names = (char**) malloc(n_files * sizeof(char*));
  for (int j = 0; j < n_files; ++j) {
    names[j] = (char*) malloc(L * sizeof(char));
  }
  int q = 0;
  while(fscanf(f, "%s", names[q]) != -1) {
    q++;
  }
  fclose(f);
  int fd[2];
  pipe(fd);
  n_threads = min(n_threads, n_files);
  for (int i = 0; i < n_threads; ++i) {
    if (!fork()) {
      close(fd[0]);
      char buf[100];
      int curr_chars = 0, curr_words = 0, curr_lines = 0;
      int lim = n_files / n_threads;
      if (i < n_files % n_threads)
        lim++;
      for (int j = 0; j < lim; ++j) {
        int fd1[2];
        pipe(fd1);
        if (!fork()) {
          dup2(fd1[1], STDOUT_FILENO);
          close(fd1[1]);
          close(fd1[0]);
          execlp("wc", "wc", names[n_threads * j + i], NULL);
          exit(EXIT_FAILURE);
        }
        close(fd1[1]);
        wait(NULL);
        read(fd1[0], buf, 100);
        close(fd1[0]);
        int w, c, l;
        sscanf(buf, "%d%d%d", &l, &w, &c);
        curr_chars += c;
        curr_words += w;
        curr_lines += l;
      }
      int res[3];
      res[2] = curr_chars;
      res[1] = curr_words;
      res[0] = curr_lines;
      write(fd[1], res, 3 * sizeof(int));
      close(fd[1]);
      exit(0);
    }
  }
  for (int j = 0; j < n_files; ++j) {
    free(names[j]);
  }
  free(names);
  close(fd[1]);
  for (int i = 0; i < n_threads; ++i) {
    wait(NULL);
  }
  int res[3];
  for (int i = 0; i < n_threads; ++i) {
    read(fd[0], res, 3* sizeof(int));
    n_lines += res[0];
    n_words += res[1];
    n_chars += res[2];
  }
  close(fd[0]);
  printf("%d %d %d\n", n_lines, n_words, n_chars);
  return 0;
}