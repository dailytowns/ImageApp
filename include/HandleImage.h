#ifndef IMAGEAPP_HANDLEIMAGE_H
#define IMAGEAPP_HANDLEIMAGE_H

#include <fcntl.h>

pthread_mutex_t fd_cache_mtx;

/**
 * Enumeration of the extensions handled by the server
 */
enum ext_image {JPG, JXR, PNG, WEBP, ALL_EXT};

/**
 * A struct that gather informations about mime-types
 */
struct image_node_t {
    /*@{*/
    char *type;                                                                                                         /**< MIME type */
    float q;                                                                                                            /**< Quality of the image requested */
    int extension;                                                                                                      /**< Integer identifying MIME type requested */
    /*@{*/
};

struct fd_cache {
    int fd[128];
    off_t file_size[128];
    pthread_mutex_t fd_cache_mtx[128];
    pthread_cond_t fd_cache_cond[128];
    int E;
};

struct fd_cache fd_image_cache;

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

void destroy_image(struct image_t *image);


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
 * @param idx_fd_cache Index in the file descriptor cache
 * @return A pointer to the list
 */
int get_image_to_send(struct image_t *image, int idx_fd_cache);
#endif //IMAGEAPP_HANDLEIMAGE_H
