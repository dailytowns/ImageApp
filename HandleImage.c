//
// Created by federico on 14/10/17.
//

#include <ImageMagick-7/MagickWand/MagickWand.h>
#include <string.h>
#include "include/HandleImage.h"
#include "include/Utils.h"
#include "include/Strings.h"
#include "include/Config.h"
#include "include/Request.h"

extern char *file_map;
extern size_t seek_cache;

ImageNode *create_mime_list(int preallocation) {

    ImageNode *node = memory_alloc(preallocation * sizeof(ImageNode));
    int i = 0;

    while (i < preallocation) {
        (node + i)->type = memory_alloc(16 * sizeof(char));
        i++;
    }

    return node;

}

int get_image_to_send(struct image_t *image) {

    int fd;
    char *image_path = NULL;

    size_t width = 0, height = 0;

    if (image->cached == CACHED_IMAGE) {
        fd = open_file(image->cache_path, O_RDONLY);
        if (fd != -1) {
            image->file_size = get_file_size(fd);
            image->fd = fd;
            return CACHED_IMAGE;
        }
    } else {
        fd = open_file(image->cache_path, O_CREAT | O_RDWR);
    }

    MagickWand *magickWand = NewMagickWand();

    MagickBooleanType result = MagickReadImage(magickWand, image_path);
    if (result == MagickFalse && !strcmp(image->ext, ".jpg")) {
        image_path = catenate_strings(IMAGE_DIR, image->image_name);
        image_path = catenate_strings(image_path, ".jpeg");
        result = MagickReadImage(magickWand, image_path);
        if (result == MagickFalse)
            return IMAGE_NOT_PRESENT;
    }

    width = image->width ? (size_t) image->width : MagickGetImageWidth(magickWand);
    height = image->height ? (size_t) image->height : MagickGetImageHeight(magickWand);

    char *format = NULL;

    if (image->image_list->extension == JPG) {
        format = (char *) memory_alloc(5);
        memcpy(format, ".jpg", 4);
        format[4] = '\0';
    } else if ((image->image_list->extension == PNG)) {
        format = (char *) memory_alloc(5);
        memcpy(format, ".png", 4);
        format[4] = '\0';
    } else if ((image->image_list->extension == JXR)) {
        format = (char *) memory_alloc(5);
        memcpy(format, ".jxr", 4);
        format[4] = '\0';
    } else if (image->image_list->extension == WEBP) {
        format = (char *) memory_alloc(6);
        memcpy(format, ".webp", 5);
        format[5] = '\0';
    }

    MagickSetImageFormat(magickWand, format);
    MagickSetCompressionQuality(magickWand, (size_t) (image->image_list[0].q * 100));                                   /* It is chosen the first, could be substituted with a selection algorithm*/
    MagickResizeImage(magickWand, width, height, LanczosFilter);

    result = MagickWriteImage(magickWand, image->cache_path);
    abort_with_error("MagickWriteImage", result == MagickFalse);

    free(image_path);

    DestroyMagickWand(magickWand);

    image->file_size = get_file_size(fd);
    memcpy(file_map + seek_cache, image->cache_path, 128);
    seek_cache = (seek_cache + strlen(image->cache_path)) % SIZE_FILE_LISTCACHE;

    image->fd = open_file(image->cache_path, O_RDONLY);

}