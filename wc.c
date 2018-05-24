#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

size_t c_total = 0, l_total = 0, w_total = 0;
size_t c_current = 0, l_current = 0, w_current = 0;

/*void count_char(int char_) {
  ++c_current;
  if (char_ == '\n') {
    ++l_current;
  }
}*/

/*bool get_next_word(FILE* file_ptr) {
  int c;
  while ((c = getc (file_ptr)) != EOF) {
    if (!isspace(c)) {
      ++w_current;
      break;
    }
    count_char(c);
  }
  do {
    count_char(c);
  } while ((c = getc (file_ptr)) != EOF && !isspace(c));
  return (c != EOF);
}*/

void process_file (FILE *file_ptr, char* filename) {
  c_current = 0, l_current = 0, w_current = 0;
  int was_space = 1;
  char str[255];
  while (fgets(str, sizeof(str), file_ptr) != NULL) {
    for (int j = 0; j < strlen(str); ++j) {
      ++c_current;
      if (str[j] == '\n') {
        ++l_current;
        was_space = 1;
      }
      if (was_space && !isspace(str[j])) {
        ++w_current;
        was_space = 0;
      }
      if (!was_space && isspace(str[j])) {
        was_space = 1;
      }
    }
  }
  c_total += c_current, l_total += l_current, w_total += w_current;
  printf("%zu %zu %zu %s\n", c_current, w_current, l_current, filename);
}

int main(int argc, char ** argv) {
  int i;
  char* filename;
  FILE* file_ptr;
  if (argc == 1) {
    process_file(stdin, "stdin");
  }
  for (i = 1; i < argc; ++i) {
    filename = argv[i];
    file_ptr = fopen(filename, "r");
    if (!file_ptr) {
      perror("Can't open file!");
      continue;
    }
    process_file(file_ptr, filename);
    fclose(file_ptr);
  }
  if (argc > 2) {
    printf("%zu %zu %zu %s\n", c_total, w_total, l_total, "total");
  }
  return 0;
}