//
// Created by federico on 31/10/17.
//

#ifndef IMAGEAPP_REQUEST_H
#define IMAGEAPP_REQUEST_H

#include "HandleImage.h"
#include "ThreadPool.h"
#include "Strings.h"

enum rqst_stat {OK, IMAGE_REQUESTED, MESSAGE_NOT_CORRECT, REQUEST_TOO_LONG,
    EMPTY_PATH, EMPTY_MESSAGE, IMAGE_NOT_PRESENT, ICON_REQUESTED, REQUEST_RECEIVED,
    HEAD_CMD, GET_CMD, ERROR_SENDING_MESSAGE, CONNECTION_CLOSED};

/**
 * It gathers the information parsed from the HTTP message received from the client
 */
struct request_t {
    int width;
    int height;
    int cmd;
    char *user_agent;
    char *image_name;
    ImageNode *image_list;
    char *cache_name;
    char *ext;
};

/**
 * Function: create_request
 *
 * Initializes a struct Request
 *
 * @return A pointer to the struct
 */
struct request_t *create_request();

/**
 * Function: receive_message
 *
 * It deals with receiving the http message from the client
 *
 * @param td The struct thread_data where the message is placed
 * @param idx The index for the buffer of messages
 * @return Error status
 */
int receive_request(struct thread_data *td, int idx);

/**
 * Function: parse_message
 *
 * It parses the message received from the client and sets all the fields of a struct Request that is returned
 *
 * @param message Message received
 * @param request Request of the client
 * @return Error status
 */
int parse_request(char *message, struct request_t **request);

int send_image(int conn_sd, struct image_t *image, int cmd);

int send_bad_request(int conn_sd);
#endif //IMAGEAPP_REQUEST_H
