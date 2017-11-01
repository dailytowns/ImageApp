//
// Created by federico on 31/10/17.
//

#ifndef IMAGEAPP_HANDLEIMAGE_H
#define IMAGEAPP_HANDLEIMAGE_H

#include <fcntl.h>

enum ext_image {JPG, JXR, PNG, WEBP, ALL_EXT};

struct image_node_t {
    char *type;
    float q;
    int extension;
};

typedef struct image_node_t ImageNode;

struct image_t {
    int width;
    int height;
    char *image_name;
    ImageNode *image_list;
    char *cache_name;
    char *ext;
    int fd;
    off_t file_size;
    int cached;
    char *cache_path;
    char *image_path;
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

int get_image_to_send(struct image_t *image);
#endif //IMAGEAPP_HANDLEIMAGE_H
