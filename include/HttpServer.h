//
// Created by federico on 31/10/17.
//

#ifndef IMAGEAPP_HTTPSERVER_H
#define IMAGEAPP_HTTPSERVER_H

#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>

#include "Config.h"

//struct pollfd array_fd[256];

struct server_t {
    struct sockaddr_in serv_addr;
    int listen_sock;
    struct pool_t *pool;
    //struct pollfd array_fd[NUM_THREAD_POOL];
};

void child_job(int idx_pool);

/**
 * Function: create_socket
 *
 * It creates a socket
 *
 * @param
 * @return Socket's file descriptor
 */
int create_socket();

/**
 * Function: set_socket_options
 *
 * It sets the options of the socket that are used in the program
 *
 * @param sock_fd The socket that will be set
 * @param keep_alive If it is passed as a parameter, the relative option will be set
 * @param reuse_addr If it is passed as a parameter, the relative option will be set
 * @return void
 */
void set_socket_options(int sockfd, int keep_alive, int reuse_addr);

/**
 * Function: set_server_address
 *
 * It sets the address family of the server, its ip address and the
 * port that is used to listen for connections
 *
 * @param server It is the struct that abstracts a server
 * @return void
 */
void set_server_address(struct server_t *server);

/**
 * Function: bind_address
 *
 * It binds the listening socket of the server with the ip address of
 * the server in order to demultiplex the packets correctly
 *
 * @param listenSock The socket used to listen for connections
 * @param servaddr The ip address of the server
 * @return void
 */
void bind_address(int listenSock, struct sockaddr_in serv_addr);

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
