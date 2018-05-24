#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

void process_file(FILE *in, char *filename, bool print_filename) {
  int count = 0;
  int pos;
  char str[255];
  int c;

  // Go to End of file
  if (fseek(in, 0, SEEK_END)) {
    perror("fseek failed");
    return;
  } else {
    pos = (int) ftell(in);

    // search for '\n' characters
    while (pos) {
      if (!fseek(in, --pos, SEEK_SET)) {
        c = fgetc(in);
        if (c == EOF || c == '\n')
          if (count++ == 10)
            break;
      } else {
        perror("fseek failed");
        return;
      }
    }
    // print last 10 lines
    if (print_filename)
      printf("%s\n", filename);
    while (fgets(str, sizeof(str), in))
      printf("%s", str);
  }
  printf("\n");
}

int main(int argc, char **argv) {
  int i;
  char *filename;
  FILE *file_ptr;
  if (argc == 1) {
    process_file(stdin, "stdin", false);
  } else if (argc >= 2) {
    for (i = 1; i < argc; ++i) {
      filename = argv[i];
      file_ptr = fopen(filename, "r");
      if (!file_ptr) {
        perror("Can't open file!");
        continue;
      }
      process_file(file_ptr, filename, argc > 2);
      fclose(file_ptr);
    }
  }
  return 0;
}