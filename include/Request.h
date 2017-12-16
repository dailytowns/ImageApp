#ifndef IMAGEAPP_REQUEST_H
#define IMAGEAPP_REQUEST_H

#include "HandleImage.h"
#include "ThreadPool.h"
#include "Strings.h"

/**
 * Enumeration of the possible states returned during handling the request, from the
 * the reception to the sending of the response
 */
enum rqst_stat {OK, IMAGE_REQUESTED, MESSAGE_NOT_CORRECT, REQUEST_TOO_LONG,
    EMPTY_PATH, EMPTY_MESSAGE, IMAGE_NOT_PRESENT, ICON_REQUESTED, REQUEST_RECEIVED,
    HEAD_CMD, GET_CMD, ERROR_SENDING_MESSAGE, CONNECTION_CLOSED};

/**
 * It gathers the information parsed from the HTTP message received from the client
 */
struct request_t {
    /*@{*/
    int width;                                                                                                          /**< Width of the image to be sent */
    int height;                                                                                                         /**< Heighh of the image to be sent */
    int cmd;                                                                                                            /**< Command sent by the client */
    char *user_agent;                                                                                                   /**< User-agent representing the client */
    char *image_name;                                                                                                   /**< Name of the image requested */
    ImageNode *image_list;                                                                                              /**< List of mime-types accepted */
    char *cache_name;                                                                                                   /**< Name of the image used to store it in cache */
    char *ext;                                                                                                          /**< Extension of the image requested */
    int colors;                                                                                                         /**< Number of colors supported */
    /*@{*/
};

/**
 * Function: create_request
 *
 * Allocates a struct Request
 *
 * @return A pointer to the struct
 */
struct request_t *create_request();

/**
 * Function: destroy_request
 *
 * It frees the memory used by the struct Request
 *
 * @return A pointer to the struct
 */
void destroy_request(struct request_t *request, int status_r);

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

/**
 * Function: send_message
 *
 * It sends only the headers over the socket if the cmd parameter is equal to HEAD_CMD, if
 * it is equal to GET_CMD it sends the image too
 *
 * @param conn_sd Socket used to send the image
 * @param image Image to be sent
 * @param cmd Commmand sent by the client
 * @return Error status
 */
int send_image(int conn_sd, struct image_t *image, int cmd);

/**
 * Function: send_bad_request
 *
 * It sends a small html page to notify the client a 400 error
 *
 * @param conn_sd Socket used to send the response
 * @return Error status
 */
int send_bad_request(int conn_sd);

/**
 * Function: send_bad_request
 *
 * It sends a small html page to notify the client a 503 error
 *
 * @param conn_sd Socket used to send the response
 * @return Error status
 */
int send_service_unavailable(int conn_sd);
#endif //IMAGEAPP_REQUEST_H
