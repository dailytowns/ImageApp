//
// Created by federico on 31/10/17.
//

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/Config.h"
#include "include/Utils.h"
#include "include/Request.h"


void *memory_alloc(size_t size) {

    void *p = NULL;
    errno = 0;
    p = calloc(1, size);

    if (p == NULL || errno != 0) {
        fprintf(stderr, "Error in memory allocation, errno %d\n", errno);
        exit(EXIT_FAILURE);
    }

    return p;
}

void image_list() {

    DIR *dirp = NULL;
    FILE *fp = NULL;

    /* Directory of the image file */
    struct dirent *dp;
    dirp = opendir(IMAGE_DIR);

    /* Open output file in write mode */
    fp = fopen(IMAGE_LIST, "w+");
    abort_with_error("fopen()", fp == NULL);

    int i = 1;
    //Read all files in directory
    while (dirp) {
        if ((dp = readdir(dirp)) != NULL) {
            //Do not include hidden files (like '.DS_STORE')
            if (strncmp(dp->d_name, ".", 1) == 0);
            else {
                fprintf(fp, "%3d) %33s\n", i, dp->d_name);
                ++i;
            }
        } else {
            closedir(dirp);//Close directory
            dirp = NULL;
        }
    }

    if (fclose(fp) != 0) {//Close file
        perror("fclose() in create_list()");
        exit(EXIT_FAILURE);
    }

}

void abort_with_error(char *caller, int cond) {

    if(cond) {
        fprintf(stderr, "Error in %s\n %s\n", caller, strerror(errno));
        exit(EXIT_FAILURE);
    }

}

FILE *open_fp(const char *path, const char *mode) {

    FILE *fp;
    errno = 0;
    fp = fopen(path, mode);

    if (fp == NULL || errno != 0) {
        fprintf(stderr, "Error opening file %s, errno %d\n", path, errno);
        fprintf(stderr, "Not present?\n");
        return NULL;
    }

    return fp;

}

int open_file(const char *path, int flags) {
    int fd = 0;
    errno = 0;
    fd = open(path, flags, 0666);

    if (fd == -1) {
        fprintf(stderr, "Error opening file %s errno %d\n", path, errno);
        //exit(EXIT_FAILURE);
    }

    return fd;
}

int parse_int(char *str) {

    int res;
    char *p;

    errno = 0;
    res = (int) strtol(str, &p, 0);
    if (errno != 0) {
        fprintf(stderr, "Error parsing dimensions\n");
        return 0;
    }

    return res;
}

off_t get_file_size(int fd) {

    off_t size = 0;

    size = lseek(fd, 0, SEEK_END);
    if (size == -1) {
        fprintf(stderr, "Error in lseek()\n");
        exit(EXIT_FAILURE);
    }

    if (lseek(fd, 0, SEEK_SET) == -1) {
        fprintf(stderr, "Error in lseek() returning at the start\n");
        exit(EXIT_FAILURE);
    }

    return size;

}

int find_file_in_cache(char *cache_path, char *map) {

    size_t len = 0;
    char buf[128];

    while(len < SIZE_FILE_LISTCACHE) {
        memcpy(buf, map + len, 128);
        if(strstr(buf, cache_path) != NULL)
            return CACHED_IMAGE;
        len += 128;
    }

    return NOT_CACHED_IMAGE;
}

ssize_t read_block(int in, char *buf, unsigned long size)
{
    unsigned long r;
    ssize_t v;

    r = 0;
    while (size > r) {
        v = read(in, buf, size - r);
        if (v == -1) {
            fprintf(stderr, "Error while reading file\n");
            exit(EXIT_FAILURE);
        }
        if (v == 0)
            return r;
        r += v;
        buf += v;
    }
    return r;
}

int write_block(int out, char *buf, unsigned long size)
{
    ssize_t v = 0;
    int error;

    while (size > 0) {
        errno = 0;
        v = write(out, buf + v, size - v);
        error = errno;
        if (v == -1 && error != EAGAIN) {
            if(error == EPIPE)
                return CONNECTION_CLOSED;
            fprintf(stderr, "%s\n", strerror(errno));
            fprintf(stderr, "Error while writing file\n");
            return ERROR_SENDING_MESSAGE;
        }
        size -= v;
        buf += v;
    }
}
