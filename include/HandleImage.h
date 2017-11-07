//
// Created by federico on 31/10/17.
//

#ifndef IMAGEAPP_HANDLEIMAGE_H
#define IMAGEAPP_HANDLEIMAGE_H

#include <fcntl.h>

enum ext_image {JPG, JXR, PNG, WEBP, ALL_EXT};

/**
 * A struct that gather informations about mime-types
 */
struct image_node_t {
    char *type;
    float q;
    int extension;
};

typedef struct image_node_t ImageNode;

/**
 * A struct that gather informations about the image that was
 * requested by the client
 */
struct image_t {
    /*@{*/
    int width;                                                                                                          /**< Width of the output image */
    int height;                                                                                                         /**< Height of the output image */
    char *image_name;                                                                                                   /**< Name of the image */
    ImageNode *image_list;                                                                                              /**< List of mime-types and relative quality factors parsed in http message */
    char *cache_name;                                                                                                   /**< Name of the image to be stored in cache */
    char *ext;                                                                                                          /**< Extension of the requested image */
    int fd;                                                                                                             /**< File descriptor of the image to be sent */
    off_t file_size;                                                                                                    /**< Size in bytes of the image to be sent */
    int cached;                                                                                                         /**< If equal to 1 the image is present in cache */
    char *cache_path;                                                                                                   /**< Path of the image stored in cache */
    char *image_path;                                                                                                   /**< Path of the image stored in images folder */
    int colors;                                                                                                         /**< Number of colors in the output image */
    /*@{*/
};

/**
 * Function: create_mime_list
 *
 * It allocates preallocation slots to store the MIME-types accepted by the client
 *
 * @param preallocation Number of slots to be preallocated
 * @return A pointer to the list
 */
ImageNode *create_mime_list(int preallocation);

/**
 * Function: get_image_to_send
 *
 * It returns the image in cache or transform the image stored in images folder
 *
 * @param image Image to be sent
 * @return A pointer to the list
 */
int get_image_to_send(struct image_t *image);
#endif //IMAGEAPP_HANDLEIMAGE_H
