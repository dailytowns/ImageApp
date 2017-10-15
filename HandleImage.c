//
// Created by federico on 14/10/17.
//

#include "include/HandleImage.h"
#include "include/Utils.h"

ImageNode *create_mime_list(int preallocation) {

    ImageNode *node = memory_alloc(preallocation * sizeof(ImageNode));
    int i = 0;

    while (i < preallocation) {
        (node + i)->type = memory_alloc(16 * sizeof(char));
        i++;
    }

    return node;

}
