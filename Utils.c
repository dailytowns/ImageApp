#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <pthread.h>
#include <math.h>

#include "include/ThreadPool.h"
#include "include/HttpServer.h"
#include "include/Config.h"
#include "include/Utils.h"
#include "include/Request.h"

int create_socket() {

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_fd == -1) {
        fprintf(stderr, "Error creating the socket\n");
        exit(EXIT_FAILURE);
    }
    return sock_fd;

}

void set_socket_options(int sockfd, int keep_alive, int reuse_addr) {

    /**
     * This option is offered by the operating system but the time parameters don't
     * respond to the purposes of this server
     *
     */
    if (keep_alive) {
        int optval = 1;
        if ((setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int))) == -1) {
            fprintf(stderr, "Error in setsockopt()\n");
            exit(EXIT_FAILURE);
        }
    }

    /**
     * The reuse of the socket works only if both the programs calling setsockopt() on the same one
     */
    if (reuse_addr) {
        int optval = 1;
        if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) == -1) {
            fprintf(stderr, "Error in setsockopt()\n");
            exit(EXIT_FAILURE);
        }
    }

}

void get_mutex(pthread_mutex_t *mtx) {

    if (pthread_mutex_lock(mtx) != 0) {
        fprintf(stderr, "Error getting mutex\n");
        exit(EXIT_FAILURE);
    }

}

void release_mutex(pthread_mutex_t *mtx) {

    if (pthread_mutex_unlock(mtx) != 0) {
        fprintf(stderr, "Error releasing mutex\n");
        exit(EXIT_FAILURE);
    }

}

void wait_cond(pthread_cond_t *cond, pthread_mutex_t *mx) {

    if (pthread_cond_wait(cond, mx) != 0) {
        perror("cond_wait");
        exit(EXIT_FAILURE);
    }

}

void signal_cond(pthread_cond_t *cond) {

    if (pthread_cond_signal(cond) != 0) {
        perror("cond_wait");
        exit(EXIT_FAILURE);
    }

}

void init_mutex(pthread_mutex_t *mtx) {

    if (pthread_mutex_init(mtx, NULL) != 0) {
        fprintf(stderr, "Error in pthread_mutex_init()\n");
        exit(EXIT_FAILURE);
    }

}

void init_cond(pthread_cond_t *cond) {

    if (pthread_cond_init(cond, NULL) != 0) {
        fprintf(stderr, "Error in pthread_mutex_init()\n");
        exit(EXIT_FAILURE);
    }

}

void bind_address(int listen_sock, struct sockaddr_in serv_addr) {

    if ((bind(listen_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) == -1) {

        fprintf(stderr, "Error in bind()\n");
        exit(EXIT_FAILURE);

    }

}

char *get_cache_file() {

    int fd;

    fd = open_file(IMAGE_CACHE_FILE, O_CREAT | O_EXCL | O_RDWR);
    if (fd == -1) {
        fd = open_file(IMAGE_CACHE_FILE, O_RDWR | O_TRUNC);
    }
    abort_with_error("lseek()", lseek(fd, SIZE_FILE_LISTCACHE, SEEK_SET) ==
                                -1);                                                                                    /* Makes a hole in the file */
    abort_with_error("write()", write(fd, &fd, 1) != 1);

    char *map = mmap(NULL, SIZE_FILE_LISTCACHE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    abort_with_error("mmap()", map ==
                               MAP_FAILED);                                                                             /* Maps cache file */

    errno = 0;
    mlock(map,
          SIZE_FILE_LISTCACHE);                                                                                         /* Keeps file in memory in order to avoid I/O operations */
    printf("%s\n", strerror(errno));

    return map;

}

FILE *get_log() {

    FILE *fp;

    time_t timer;
    char time_buf[26];
    struct tm *tm_info;

    time(&timer);
    tm_info = localtime(&timer);
    strftime(time_buf, 26, "%Y-%m-%d", tm_info);
    char *path = catenate_strings(SERVER_LOG_PATH, time_buf);

    fp = fopen(path, "a+");
    if (fp == NULL) {
        abort_with_error("fopen", fp == NULL);
    }

    free(path);
    return fp;
}

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
        fprintf(stderr, "Error in %s\n%s\n", caller, strerror(errno));
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

    fprintf(stderr, "IN OPEN FILE\n\n\n\n");

    int fd = 0;
    errno = 0;
    fd = open(path, flags, 0666);

    if (fd == -1) {
        fprintf(stderr, "Error opening file %s errno %d\n", path, errno);
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

int find_file_in_cache(char *cache_path, char *map, int *idx) {

    size_t len = 0;
    char buf[128];
    int i = 0;

    if(strlen(map) != 0) {
        while (len < SIZE_FILE_LISTCACHE) {

            strcpy(buf, map + len);

            if (strstr(buf, cache_path) != NULL) {
                *idx = i;
                return CACHED_IMAGE;
            }

            len += strlen(buf) + 1;
            bzero(buf, strlen(buf));
            i++;

        }
    }

    *idx = -1;
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