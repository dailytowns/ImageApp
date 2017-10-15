//
// Created by federico on 14/10/17.
//

#ifndef IMAGEAPP_STRINGS_H
#define IMAGEAPP_STRINGS_H

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
 */
void build_image_name_cache(char **cache_name, char *image_name, float q, int width, int height);

char *convert_long_to_string(long v);

char *catenate_strings(char *s1, char *s2);
#endif //IMAGEAPP_STRINGS_H
