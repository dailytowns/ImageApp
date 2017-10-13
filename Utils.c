//
// Created by federico on 13/10/17.
//

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "include/Config.h"
#include <errno.h>

#include "include/Utils.h"

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
    if(fp == NULL) abort_with_error("fopen()", 1, 1);

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

void abort_with_error(char *caller, int returned, int error_checking) {

    if(returned == error_checking) {
        fprintf(stderr, "Error in %s\n", caller);
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