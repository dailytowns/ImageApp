//
// Created by federico on 13/10/17.
//

#ifndef IMAGEAPP_MESSAGE_H
#define IMAGEAPP_MESSAGE_H

#include "ThreadPool.h"
#include "Message.h"
#include "HandleImage.h"

enum extension {ALL_EXT, JPG, PNG, WEBP, JXR};

enum cmd {GET_CMD, HEAD_CMD};

enum msg_stat {REQUEST_RECEIVED, EMPTY_MESSAGE, ERROR_SENDING_MESSAGE, CONNECTION_CLOSED};

enum rqst_stat {OK, IMAGE_REQUESTED, MESSAGE_NOT_CORRECT, REQUEST_TOO_LONG,
                EMPTY_PATH, IMAGE_NOT_PRESENT, ICON_REQUESTED};

struct Request {
    int width;
    int height;
    int cmd;
    char *user_agent;
    char *image_name;
    char *image_path;
    ImageNode *image_list;
    char *cache_name;
    char *ext;
};

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
 * Function: send_image
 *
 * It sends the image to the client
 *
 * @param conn_sd Socket of the connection
 * @param image struct of the image to be sent
 * @param cmd Set to GET_CMD or HEAD_CMD
 * @return Error status
 */
int send_image(int conn_sd, struct image_t *image, int cmd);

/**
 * Function: create_request
 *
 * Initializes a struct Request
 *
 * @return A pointer to the struct
 */
struct Request *create_request();

/**
 * Function: receive_message
 *
 * It deals with receiving the http message from the client
 *
 * @param td The struct thread_data where the message is placed
 * @param idx The index for the buffer of messages
 * @return Error status
 */
int receive_message(struct thread_data *td, int idx);

/**
 * Function: parse_message
 *
 * It parses the message received from the client and sets all the fields of a struct Request that is returned
 *
 * @param message Message received
 * @param request Request of the client
 * @return Error status
 */
int parse_message(char *message, struct Request **request);

int send_bad_request(int conn_sd);
#endif //IMAGEAPP_MESSAGE_H
