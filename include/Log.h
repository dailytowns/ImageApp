//
// Created by federico on 13/10/17.
//

#ifndef IMAGEAPP_LOG_H
#define IMAGEAPP_LOG_H

#include <netinet/in.h>

enum event_type {CONNECTION_ACCEPTED};

/**
 * Function: write_event_log
 *
 * It writes in the log file an action specified in the parameters
 *
 * @param
 * @return Socket's file descriptor
 */
void write_event_log(FILE *fp, int event, struct sockaddr_in sockaddrIn);

#endif //IMAGEAPP_LOG_H
