//
// Created by anton on 20.05.18.
//

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <zconf.h>

void traverse(char const *dir){
  char name[PATH_MAX];
  DIR *d;
  struct dirent *dd;
  off_t o;
  struct stat s;
  char* delim = "/";
  if (!strcmp(dir, "/"))
    delim = "";
  d = opendir(dir);
  while ((dd = readdir(d))) {
    if (!strcmp(dd->d_name, ".") || !strcmp(dd->d_name, ".."))
      continue;
    snprintf(name, PATH_MAX, "%s%s%s", dir, delim, dd->d_name);
    if (lstat(name, &s) < 0)
      continue;
    if (S_ISDIR(s.st_mode)) {
      o = telldir(d);
      closedir(d);
      traverse(name);
      d = opendir(dir);
      seekdir(d, o);
    } else {
      int fd = open(name, O_RDONLY);
      char buf[512];
      ssize_t n;
      while ((n = read(fd, buf, 512)) > 0)
        write(1, buf, n);
    }
  }
  closedir(d);
}

int main() {
  traverse("./test");
  return 0;
}