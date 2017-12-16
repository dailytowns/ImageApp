#include <ImageMagick-7/MagickWand/MagickWand.h>
#include <string.h>

#include "include/HandleImage.h"
#include "include/Utils.h"
#include "include/Strings.h"
#include "include/Config.h"
#include "include/Request.h"

extern char *file_map;
extern size_t seek_cache;


void destroy_image(struct image_t *image_info) {

    free(image_info->image_path);
    free(image_info->cache_path);
    free(image_info->ext);
    free(image_info);

}

ImageNode *create_mime_list(int preallocation) {

    ImageNode *node = memory_alloc(preallocation * sizeof(ImageNode));
    int i = 0;

    while (i < preallocation) {
        (node + i)->type = memory_alloc(16 * sizeof(char));
        i++;
    }

    return node;

}

int get_image_to_send(struct image_t *image, int idx_fd_cache) {

    int fd;
    char *image_path = NULL, *tmp_image_path = NULL;

    size_t width = 0, height = 0, colors = 0;

    if (image->cached == CACHED_IMAGE) {

        //fd = open_file(image->cache_path, O_RDONLY);
        //fd_cache[idx_fd_cache];
        //if (fd != -1) {
            image->file_size = fd_image_cache.file_size[idx_fd_cache];
            image->fd = fd_image_cache.fd[idx_fd_cache];
            return CACHED_IMAGE;
        //}

    } else {
        get_mutex(&fd_cache_mtx);
            fd = open_file(image->cache_path, O_CREAT | O_RDWR);
        release_mutex(&fd_cache_mtx);
    }

    MagickWand *magickWand = NewMagickWand();

    MagickBooleanType result = MagickReadImage(magickWand, image->image_path);
    if (result == MagickFalse && !strcmp(image->ext, ".jpg")) {

        tmp_image_path = catenate_strings(IMAGE_DIR, image->image_name);
        image_path = catenate_strings(tmp_image_path, ".jpeg");
        result = MagickReadImage(magickWand, image_path);

        if (result == MagickFalse) {
            free(tmp_image_path);
            free(image_path);
            return IMAGE_NOT_PRESENT;
        }

    }

    width = image->width ? (size_t) image->width : MagickGetImageWidth(magickWand);
    height = image->height ? (size_t) image->height : MagickGetImageHeight(magickWand);
    colors = image->colors ? (size_t) image->colors : MagickGetImageColors(magickWand);


    char *format = NULL;

    if (image->image_list->extension == JPG) {
        format = memory_alloc(5);
        memcpy(format, ".jpg", 4);
        format[4] = '\0';
    } else if ((image->image_list->extension == PNG)) {
        format = memory_alloc(5);
        memcpy(format, ".png", 4);
        format[4] = '\0';
    } else if ((image->image_list->extension == JXR)) {
        format = memory_alloc(5);
        memcpy(format, ".jxr", 4);
        format[4] = '\0';
    } else if (image->image_list->extension == WEBP) {
        format = memory_alloc(6);
        memcpy(format, ".webp", 5);
        format[5] = '\0';
    } else if (image->image_list->extension == ALL_EXT) {
        format = memory_alloc(5);
        memcpy(format, ".jpg", 4);
        format[4] = '\0';
    }

    MagickSetImageFormat(magickWand, format);
    MagickSetCompressionQuality(magickWand, (size_t) (image->image_list[0].q * 100));                                   /* It is chosen the first, could be substituted with a selection algorithm*/
    MagickResizeImage(magickWand, width, height, LanczosFilter);
    MagickQuantizeImage(magickWand, colors, RGBColorspace, 8, DitherVirtualPixelMethod, MagickFalse);

    result = MagickWriteImage(magickWand, image->cache_path);
    abort_with_error("MagickWriteImage", result == MagickFalse);

    ClearMagickWand(magickWand);
    DestroyMagickWand(magickWand);

    image->file_size = get_file_size(fd);

    /*memcpy() is thread-safe but the others are not */
    get_mutex(&fd_cache_mtx);
        memcpy(file_map + seek_cache, image->cache_path, strlen(image->cache_path));
        file_map[seek_cache + strlen(image->cache_path)] = '\0';
        seek_cache = (seek_cache + strlen(image->cache_path) + 1) % SIZE_FILE_LISTCACHE;
        fd_image_cache.fd[fd_image_cache.E] = fd;
        fd_image_cache.file_size[fd_image_cache.E] = image->file_size;
        fd_image_cache.E = (fd_image_cache.E + 1) % 128;
    release_mutex(&fd_cache_mtx);

    //image->fd = open_file(image->cache_path, O_RDONLY);
    image->fd = fd;

    free(tmp_image_path);
    free(image_path);
}