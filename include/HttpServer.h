//
// Created by federico on 29/06/17.
//

#ifndef IMAGEAPP_HTTPSERVER_H
#define IMAGEAPP_HTTPSERVER_H

#include <sys/types.h>


struct Server {
    struct sockaddr_in serv_addr;
    int listen_sock;
    struct thread_pool *thread_pool;
    struct pollfd array_fd[NUM_THREAD_POOL];
    FILE *log_fp;

    void (*set_server_address)(struct Server* serverPtr);
    void (*set_socket_options)(int sock_fd, int keep_alive, int reuse_addr);
    void (*bind_address)(int listenSock, struct sockaddr_in serv_addr);
    void (*image_list)();
    void (*init_pollfd)(struct Server* serverPtr);
    struct thread_pool *(*allocate_pool)(int num_thr);
};

typedef struct Server* ServerPtr;

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
void set_server_address(struct Server *server);

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
ServerPtr create_server();

ServerPtr Server();
#endif //IMAGEAPP_HTTPSERVER_H
