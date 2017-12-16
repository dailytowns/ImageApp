#ifndef IMAGEAPP_STRINGS_H
#define IMAGEAPP_STRINGS_H

#include <sys/types.h>

/**
 * Function: build_image_name_cache
 *
 * This function builds the name used to store and retrieve an image from cache requested from client.
 * The information used to perform the operation are the name of the image, quality requested, width
 * and height of the display requesting.
 *
 * @param cache_name Pointer to the result string
 * @param image_name Name of the image
 * @param q Quality factory requested
 * @param width Width of the display requesting
 * @param height Width of the display requesting
 * @param colors Number of colors supported by the client
 */
void build_image_name_cache(char **cache_name, char *image_name, float q, int width, int height, int colors);

/**
 * Function: convert_long_to_string
 *
 * This function converts a long to a string in order to be printed in a response message
 *
 * @param v Value to be converted
 * @return An array of char containing the conversion
 */
char *convert_long_to_string(long v);

/**
 * Function: convert_int_to_string
 *
 * This function converts an int to a string in order to be printed in a response message
 *
 * @param v Value to be converted
 * @return An array of char containing the conversion
 */
char *convert_int_to_string(int v);

/**
 * Function: catenate_string
 *
 * This function catenates two strings passed as parameters
 *
 * @param s1 First string
 * @param s2 Second string
 * @return An array of char containing the catenation
 */
char *catenate_strings(char *s1, char *s2);

/**
 * Function: build_message
 *
 * This function builds the response message that has to be sent to the client
 *
 * @param
 * @param s2 Second string
 * @return An array of char containing the catenation
 */
size_t build_message(off_t file_size, char *ext, char msg[], struct image_t *image, char *msg_error);

/**
 * Function: convert_float_to_string
 *
 * This function converts a float to a string
 *
 * @param n Value to be converted
 * @return An array of char containing the conversion
 */
char *convert_float_to_string(float n);
#endif //IMAGEAPP_STRINGS_H
