//
// Created by federico on 13/10/17.
//

#ifndef IMAGEAPP_MESSAGE_H
#define IMAGEAPP_MESSAGE_H

#include "ThreadPool.h"
#include "Message.h"
#include "HandleImage.h"

enum msg_stat {REQUEST_RECEIVED, EMPTY_MESSAGE, CONNECTION_CLOSED};

enum rqst_stat {OK, ICON_REQUESTED, IMAGE_REQUESTED, MESSAGE_NOT_CORRECT, REQUEST_TOO_LONG};

struct Request {
    int width;
    int height;
    int cmd;
    char *user_agent;
    char *image_name;
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
};

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
 * @return Error status
 */
int parse_message(char *message, struct Request **request);
#endif //IMAGEAPP_MESSAGE_H
