#include <stdio.h>
#include <arpa/inet.h>
#include <time.h>

#include "include/Log.h"
#include "include/Request.h"

void write_event_log(FILE *log, int event, struct sockaddr_in sockaddrIn, struct image_t *image) {

    time_t timer;
    char time_buf[50];
    struct tm tm_info;

    timer = time(&timer);
    localtime_r(&timer, &tm_info);                                                                                      /* Thread safe version of localtime() */

    strftime(time_buf, 26, "%Y-%m-%d %H:%M:%S", &tm_info);

    switch (event) {

        case CONNECTION_ACCEPTED:
            fprintf(log, "<<%s>> : a client has connected. Its address is %s and was given %d port\n",
                    time_buf, inet_ntoa(sockaddrIn.sin_addr), ntohs(sockaddrIn.sin_port));
            fflush(log);
            break;

        case LOG_IMAGE_REQUESTED:
            fprintf(log, "<<%s>> : the client %s has requested %s image\n",
                    time_buf, inet_ntoa(sockaddrIn.sin_addr), image->cache_path);
            fflush(log);
            break;

        case LOG_EMPTY_MESSAGE:
            fprintf(log, "<<%s>> : the client %s has disconnected\n",
                    time_buf, inet_ntoa(sockaddrIn.sin_addr));
            fflush(log);
            break;

        case LOG_MAX_NUM_REQUESTS:
            fprintf(log, "<<%s>> : the client %s has finished its number of requests\n",
                    time_buf, inet_ntoa(sockaddrIn.sin_addr));
            fflush(log);
            break;

        case LOG_IMAGE_NOT_PRESENT:
            fprintf(log, "<<%s>> : the client %s has requested the image %s that is not present\n",
                    time_buf, inet_ntoa(sockaddrIn.sin_addr), image->image_name);
            fflush(log);
            break;

        case CONNECTION_CLOSED:
            fprintf(log, "<<%s>> : a client has disconnected. Its address was %s and was given %d port\n",
                    time_buf, inet_ntoa(sockaddrIn.sin_addr), ntohs(sockaddrIn.sin_port));
            fflush(log);
            break;

        default:
            break;
    }

}