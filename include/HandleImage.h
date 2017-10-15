//
// Created by federico on 14/10/17.
//

#ifndef IMAGEAPP_HANDLEIMAGE_H
#define IMAGEAPP_HANDLEIMAGE_H

struct image_node_t {
    char *type;
    float q;
};

typedef struct image_node_t ImageNode;

/**
 * Function: create_mime_list
 *
 * It allocates preallocation slots to store the MIME-types accepted by the client
 *
 * @param preallocation Number of slots to be preallocated
 * @return A pointer to the list
 */
ImageNode *create_mime_list(int preallocation);


#endif //IMAGEAPP_HANDLEIMAGE_H
