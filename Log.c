//
// Created by federico on 13/10/17.
//

#include <stdio.h>
#include <arpa/inet.h>
#include <time.h>

#include "include/Log.h"

void write_event_log(FILE *log, int event, struct sockaddr_in sockaddrIn, struct image_t *image) {

    time_t timer;
    char time_buf[26];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(time_buf, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    switch (event) {
        case CONNECTION_ACCEPTED:
            fprintf(log, "<<%s>> : a client has connected. Its address is %s and was given %d port\n",
                    time_buf, inet_ntoa(sockaddrIn.sin_addr), ntohs(sockaddrIn.sin_port));
            fflush(log);
            break;

        case IMAGE_REQUESTED:
            fprintf(log, "<<%s>> : the client %s has requested %s image\n",
                    time_buf, inet_ntoa(sockaddrIn.sin_addr), image->cache_path);
            fflush(log);
            break;

        default:
            break;
    }


}