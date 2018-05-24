#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <stdbool.h>

#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW 256
#endif

int fstatat(int dirfd, const char *pathname, struct stat *buf, int flags);
int dirfd(DIR *dirp);
typedef struct dirent dirent_t;
typedef struct stat stat_t;

char* build_path(const char* dirname, const char* filename) {
    size_t dirname_len = strlen(dirname);
    size_t fname_len = strlen(filename);
    char* buf = malloc(dirname_len + fname_len + 2);
    buf[0] = '\0';
    strcat(buf, dirname);
    if (buf[dirname_len - 1] != '/') {
        buf[dirname_len] = '/';
        buf[dirname_len + 1] = '\0';
    }
    strcat(buf, filename);
    return buf;
}

void perform_search(const char* dirname, const char* fname, uid_t uid) {
    DIR* dir = opendir(dirname);
    if (dir == NULL) {
        error(0, errno, "Cannot open directory %s", dirname);
        return;
    }
    int dir_fd = dirfd(dir);
    if (dir_fd < 0) {
        error(0, errno, "Cannot open directory %s", dirname);
    }
    dirent_t* entry;
    entry = readdir(dir);

    while (entry != NULL) {
        const char* cur_fname = entry->d_name;
        char* full_name = NULL;

        stat_t s;
        if (fstatat(dir_fd, cur_fname, &s, AT_SYMLINK_NOFOLLOW)) {
            error(0, errno, "Error while processing %s", cur_fname);
        }

        if (strcmp(fname, cur_fname) == 0 && s.st_uid == uid) {
            full_name = build_path(dirname, cur_fname);
            printf("%s\n", full_name);
        }

        size_t len_cur_fname = strlen(cur_fname);
        bool is_loopback = len_cur_fname <= 2 && cur_fname[0] == '.' && cur_fname[len_cur_fname - 1] == '.';

        if (S_ISDIR(s.st_mode) && !is_loopback) {
            if (full_name == NULL) {
                full_name = build_path(dirname, cur_fname);
            }
            perform_search(full_name, fname, uid);
        }


        if (full_name != NULL) {
            free(full_name);
        }
        entry = readdir(dir);
    }

    if (errno == EBADF) {
        error(0, EBADF, "Invalid directory descriptor");
    }

    if (closedir(dir) && errno == EBADF) {
        error(0, EBADF, "Invalid directory descriptor");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <root_dir> <name> <user>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct passwd* pwd = getpwnam(argv[3]);
    if (pwd == NULL) {
        error(1, errno, "No such user: %s", argv[3]);
    }
    uid_t uid = pwd->pw_uid;
    perform_search(argv[1], argv[2], uid);

    return EXIT_SUCCESS;
}
