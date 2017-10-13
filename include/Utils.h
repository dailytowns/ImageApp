//
// Created by federico on 13/10/17.
//

#ifndef IMAGEAPP_UTILS_H
#define IMAGEAPP_UTILS_H

#include <sys/types.h>

/**
 * Function: abort_with_error
 *
 * It aborts the program if the values are equal and prints in the stderr
 * an error message
 *
 * @param Function that could fail
 * @param returned A value returned by a function
 * @param error_checking The error value returned by a function
 * @return A pointer that points the memory allocated
 */
void abort_with_error(char *caller, int returned, int error_checking);

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
#endif //IMAGEAPP_UTILS_H
