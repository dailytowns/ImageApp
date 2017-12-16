//
// Created by federico on 31/10/17.
//

#ifndef IMAGEAPP_HTTPSERVER_H
#define IMAGEAPP_HTTPSERVER_H

#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <semaphore.h>

#include "Config.h"

pthread_mutex_t mtx_realloc;
pthread_mutex_t mtx_realloc_end;
pthread_cond_t cond_realloc;
pthread_cond_t cond_realloc_end;
pthread_mutex_t mtx_accept;

struct dispatcher *dispatcher;
struct pool_t **pool;

struct server_t {
    struct sockaddr_in serv_addr;
    int listen_sock;
    struct pool_t *pool;
};

/**
 * Function: handle_client
 *
 * Function that deals with receiving message from the client, parsing it and sending
 * the requested image
 *
 * @param arg The struct thread_data * containing fields used to correctly run the operation
 * @return void *
 */
void *handle_client(void *arg);

/**
 * Function: create_server
 *
 * It initializes all the fields of the server struct
 *
 * @param
 * @return A struct Server * ready to be used
 */
struct server_t *init_server();

#endif //IMAGEAPP_HTTPSERVER_H
