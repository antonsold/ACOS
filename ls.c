#undef __STRICT_ANSI__
#define __USE_XOPEN
#define __USE_ATFILE

#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdlib.h>

#define RWX_STRING      "rwxrwxrwx"
#define TIME_FORMAT     "%b %d %Y %H:%M"

int fstatat(int dirfd, const char *pathname, struct stat *buf, int flags);
int dirfd(DIR *dirp);

typedef enum {
    COL_MODE,
    COL_NLINK,
    COL_USER,
    COL_GROUP,
    COL_FSIZE,
    COL_CTIME,
    COL_FNAME,
    COL_TOTAL_COUNT
} col_entry_t;

typedef struct dirent dirent_t;
typedef struct stat stat_t;
typedef struct timespec timespec_t;

static inline unsigned long long max_u64(unsigned long long a, unsigned long long b) {
    return a < b ? b : a;
}

static inline unsigned int int_length(unsigned long long n) {
    unsigned int ans = 1;
    while (n >= 10) {
        ++ans;
        n /= 10;
    }
    return ans;
}

char filetype_prefix(mode_t st_mode) {
    switch (st_mode) {
        case S_IFDIR: return 'd';
        case S_IFCHR: return 'c';
        case S_IFBLK: return 'b';
        case S_IFLNK: return 'l';
        default: return '-';
    }
}

const char *rwx(unsigned long mode) {
    static char str[10];
    str[9] = 0;
    for (int i = 0, j = 1 << 8; i < 9; ++i, j >>= 1) {
        str[i] = (mode & j) ? RWX_STRING[i] : '-';
    }
    return str;
}

const char *get_change_time(const stat_t *stat) {
    static char str[20];
    strftime(str, 20, TIME_FORMAT, localtime(&stat->st_ctime));
    return str;
}

void prepare_table(DIR *dir, const dirent_t *entry, unsigned int *table) {
    const char *fname = entry->d_name;

    stat_t s;
    int fd = dirfd(dir);
    if (fd < 0) {
        error(1, errno, "Error while processing %s", fname);
    }
    if (fstatat(fd, fname, &s, AT_SYMLINK_NOFOLLOW)) {
        error(1, errno, "Error while processing %s", fname);
    }
    table[COL_NLINK] = max_u64(table[COL_NLINK], int_length(s.st_nlink));
    table[COL_USER]  = max_u64(table[COL_USER],  strlen(getpwuid(s.st_uid)->pw_name));
    table[COL_GROUP] = max_u64(table[COL_GROUP], strlen(getgrgid(s.st_gid)->gr_name));
    table[COL_FSIZE] = max_u64(table[COL_FSIZE], int_length(s.st_size));
    table[COL_CTIME] = max_u64(table[COL_CTIME], strlen(get_change_time(&s)));
}

void print_symlink(int dirfd, const char *fname) {
    int strsz = 2 * PATH_MAX + 1;
    char *linkname = malloc(strsz);
    if (linkname == NULL) {
        error(1, errno, "Error while reading symlink");
    }
    int r;
    if (dirfd < 0) {
        r = readlink(fname, linkname, strsz);
    } else {
        r = readlinkat(dirfd, fname, linkname, strsz);
    }
    if (r < 0) {
        error(1, errno, "Error while reading symlink");
    }

    linkname[r < strsz ? r : strsz] = '\0';
    fputs(linkname, stdout);
    free(linkname);
}

void process_file(int dir_fd, const char *fname, const stat_t *s, unsigned int *table) {
    printf("%c%s %*ld %*s %*s %*ld %*s ",
            filetype_prefix(s->st_mode & S_IFMT), rwx(s->st_mode),
            table[COL_NLINK], s->st_nlink,
            table[COL_USER],  getpwuid(s->st_uid)->pw_name,
            table[COL_GROUP], getgrgid(s->st_gid)->gr_name,
            table[COL_FSIZE], s->st_size,
            table[COL_CTIME], get_change_time(s));
    printf("%s", fname);
    if (S_ISLNK(s->st_mode)) {
        printf(" -> ");
        print_symlink(dir_fd, fname);
    }
    printf("\n");
}

void process_direntry(DIR *dir, const dirent_t *entry, unsigned int *table) {
    const char *fname = entry->d_name;

    stat_t s;
    int fd = dirfd(dir);
    if (fd < 0) {
        error(1, errno, "Error while processing %s", fname);
    }
    if (fstatat(fd, fname, &s, AT_SYMLINK_NOFOLLOW)) {
        error(1, errno, "Error while processing %s", fname);
    }

    process_file(fd, fname, &s, table);
}

void process_dir(const char *dirname) {
    printf("%s:\n", dirname);
    unsigned int table_columns_width[COL_TOTAL_COUNT];
    for (int i = 0; i < COL_TOTAL_COUNT; ++i) {
        table_columns_width[i] = 0;
    }

    DIR *dir;
    dirent_t *entry;

    dir = opendir(dirname);
    if (dir == NULL) {
        /* Try to open as file */
        stat_t s;
        lstat(dirname, &s);
        process_file(-1, dirname, &s, table_columns_width);
        return;
    }

    entry = readdir(dir);
    while (entry != NULL) {
        prepare_table(dir, entry, table_columns_width);
        entry = readdir(dir);
    }
    if (errno == EBADF) {
        error(1, EBADF, "Invalid directory descriptor");
    }
    rewinddir(dir);

    entry = readdir(dir);
    while (entry != NULL) {
        process_direntry(dir, entry, table_columns_width);
        entry = readdir(dir);
    }
    if (errno == EBADF) {
        error(1, EBADF, "Invalid directory descriptor");
    }
    rewinddir(dir);

    entry = readdir(dir);
    size_t len_dirname = strlen(dirname);
    while (entry != NULL) {
        stat_t s;
        const char *fname = entry->d_name;
        int fd = dirfd(dir);
        if (fd < 0) {
            error(1, errno, "Error while processing %s", fname);
        }
        if (fstatat(fd, fname, &s, AT_SYMLINK_NOFOLLOW)) {
            error(1, errno, "Error while processing %s", fname);
        }

        size_t len_fname = strlen(fname);
        bool is_loopback = len_fname <= 2 && fname[0] == '.' && fname[len_fname - 1] == '.';

        if (S_ISDIR(s.st_mode) && !is_loopback) {
          size_t len = len_dirname + len_fname + 5;
          char *fullname = malloc(len);
          fullname[0] = '\0';
          strcat(fullname, dirname);
          if (fullname[len_dirname - 1] != '/') {
            fullname[len_dirname] = '/';
            fullname[len_dirname + 1] = '\0';
          }
          strcat(fullname, fname);
          printf("\n");
          process_dir(fullname);
          free(fullname);
        }
        entry = readdir(dir);
    }
    if (errno == EBADF) {
        error(1, EBADF, "Invalid directory descriptor");
    }

    if (closedir(dir) && errno == EBADF) {
        error(1, EBADF, "Invalid directory descriptor");
    }
}

int main(int argc, char *argv[]) {
    int processed_dirs_cnt = 0;

    for (int i = 1; i < argc; ++i) {
        process_dir(argv[i]);
        printf("\n");
        ++processed_dirs_cnt;
    }

    if (!processed_dirs_cnt) {
        process_dir("./");
    }

    return EXIT_SUCCESS;
}
