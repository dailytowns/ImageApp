#ifndef IMAGEAPP_LOG_H
#define IMAGEAPP_LOG_H

#include <netinet/in.h>
#include <stdio.h>

#include "HandleImage.h"

/**
 * Enumeration that is used to choose the action to be tracked in the log
 */
enum event_type {CONNECTION_ACCEPTED, LOG_IMAGE_REQUESTED, LOG_EMPTY_MESSAGE, LOG_MAX_NUM_REQUESTS, LOG_IMAGE_NOT_PRESENT};

/**
 * Function: write_event_log
 *
 * It writes in the log file an action specified in the parameters
 *
 * @param fp file pointer of the log
 * @param event event to be stored in log
 * @param sockaddrIn address of the client
 * @param image struct of an image requested, if any
 * @return Socket's file descriptor
 */
void write_event_log(FILE *fp, int event, struct sockaddr_in client_addr, struct image_t *image);

#endif //IMAGEAPP_LOG_H
