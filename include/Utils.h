//
// Created by federico on 13/10/17.
//

#ifndef IMAGEAPP_UTILS_H
#define IMAGEAPP_UTILS_H

#include <sys/types.h>
#include <stdio.h>

enum cache_file {CACHED_IMAGE, NOT_CACHED_IMAGE};

/**
 * Function: abort_with_error
 *
 * It aborts the program if the values are equal and prints in the stderr
 * an error message
 *
 * @param Function that could fail
 * @param cond If verified, the program is aborted
 * @return A pointer that points the memory allocated
 */
void abort_with_error(char *caller, int cond);

/**
 * Function: memory_alloc
 *
 * It deals with the memory allocation and the error detection in
 * this operation
 *
 * @param size Number of bytes to be allocated
 * @return A pointer that points the memory allocated
 */
void *memory_alloc(size_t size);

/**
 * Function: image_list
 *
 * It deals with the creation of the list of files present in the image
 * directory
 *
 * @param
 * @return void
 */
void image_list();

/**
 * Function: open_fp
 *
 * It opens a file and checks for error in this operation
 *
 * @param path Path of the file to be opened
 * @param mode It specifies the modality of using the file.
 * @return A file pointer pointing to the file
 */
FILE *open_fp(const char *path, const char *mode);

/**
 * Function: open_file
 *
 * It opens a file and checks for error in this operation
 *
 * @param path Path of the file to be opened
 * @param mode It specifies the modality of using the file.
 * @return A file descriptor of the file
 */
int open_file(const char *path, int mode);

/**
 * Function: parse_int
 *
 * It parses a string to an integer
 *
 * @param s String to be parsed
 * @return Integer corresponding to s
 */
int parse_int(char *s);

/**
 * Function: get_file_size
 *
 * Computes the size of the file
 *
 * @param fd file descriptor
 * @return file size
 */
off_t get_file_size(int fd);

/**
 * Function: find_file_in_cache
 *
 * Scan cache to check if image requested has been
 * requested before
 *
 * @param cache_path Path in cache
 * @param map mapped file
 * @return status code
 */
int find_file_in_cache(char *cache_path, char *map);
#endif //IMAGEAPP_UTILS_H
