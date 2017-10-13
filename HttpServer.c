#include <poll.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <ImageMagick-7/MagickWand/MagickWand.h>
#include <string.h>
#include <errno.h>

#include "include/Config.h"
#include "include/HttpServer.h"
#include "include/ThreadPool.h"
#include "include/Utils.h"
#include "include/Log.h"

int main(int argc, char *argv[]) {

    MagickWandGenesis();                                                                                                /* Initializes the MagickWand environment */

    int conn_sd;                                                                                                        /* Connection's file descriptor */
    int ready;                                                                                                          /* Number of ready fd */
    int index;                                                                                                          /* Iterators */
    int ret;
    nfds_t max_descriptor = 1, d;                                                                                       /* Number of descriptors to be checked */


    socklen_t socklen;
    struct sockaddr_in client_addr;

    ServerPtr serverPtr = create_server();
    struct thread_pool *thread_pool = serverPtr->thread_pool;
    struct thread_data *thread_data = thread_pool->td_pool;

    /*****************************    Main loop   ***********************************************************/
    while (1) {

        ready = poll(serverPtr->array_fd, max_descriptor, 1000);

        switch (ready) {
            case -1:                                                                                                    /* If an error occurs in poll() */
                fprintf(stderr, "Error in poll()\n");
                if (errno == EBADF || errno == ENOMEM) {
                    fprintf(stderr, "in select, errno %d\n", errno);
                    exit(EXIT_FAILURE);
                }
                continue;

            case 0:                                                                                                     /* If no fd is ready to be read */
                index = 0;
                while (index < max_descriptor)
                    printf("descriptor %d %d\n", index, serverPtr->array_fd[index++].fd);
                continue;

            default:
                break;
        }

        for (d = 0; d <= max_descriptor; d++) {

            if (serverPtr->array_fd[d].revents &
                POLLIN) {                                                                                               /* If the d-ith descriptor has received bytes to be read */

                if (serverPtr->array_fd[d].fd ==
                    serverPtr->listen_sock) {                                                                           /* If the descriptor chosen is the listen socket, there is new connection */

                    socklen = sizeof(client_addr);
                    conn_sd = accept(serverPtr->listen_sock,
                                     (struct sockaddr *) &client_addr,
                                     &socklen);
                    abort_with_error("accept()", conn_sd, -1);

                    set_socket_options(conn_sd, SO_KEEPALIVE, 0);

                    /**************** Searching for a free slot *************************************/
                    get_mutex(&(serverPtr->thread_pool->mtx));

                    int nE = get_E(
                            thread_pool,
                            thread_pool->S,
                            thread_pool->E);                                                                            /* Finds a free slot in the ring buffer */
                    thread_pool->E = nE;
                    printf("%d\n", thread_pool->E);

                    thread_pool->td_pool[nE].conn_sd = conn_sd;                                                         /* The rest of assignments are done in allocate_pool() */
                    thread_pool->td_pool[nE].E = nE;

                    release_mutex(&(serverPtr->thread_pool->mtx));
                    /******************************************************************************/

                    write_event_log(serverPtr->log_fp, CONNECTION_ACCEPTED, client_addr);                               /* Keeps track of the client's connection in the log file */

                    ret = pthread_create(&(thread_data[nE].tid), NULL, handle_client, &(thread_data[nE]));
                    if(ret) abort_with_error("pthread_create()\n", 1, 1);

                }

            }

        }

    }
    /**********************************************************************************************************/
    return EXIT_SUCCESS;
}

void *handle_client(void *arg) {

    struct thread_data *td = (struct thread_data *)arg;

    MagickWand *magickWand = NewMagickWand();

    MagickReadImage(magickWand, "/home/federico/CLionProjects/Swap/images/meganfox.jpg");

    ClearMagickWand(magickWand);

}

int create_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(stderr, "Error creating the socket\n");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void set_socket_options(int sockfd, int keep_alive, int reuse_addr) {

    /**
    * Il riutilizzo dell'indirizzo funziona solo se entrambi i programmi
    * invocano setsockopt() sulla stessa socket
    */
    if (keep_alive) {
        int optval = 1;
        if ((setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int))) == -1) {
            fprintf(stderr, "Error in setsockopt()\n");
            exit(EXIT_FAILURE);
        }
    }

    if (reuse_addr) {
        int optval = 1;
        if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int))) == -1) {
            fprintf(stderr, "Error in setsockopt()\n");
            exit(EXIT_FAILURE);
        }
    }

}

void set_server_address(ServerPtr serverPtr) {

    struct sockaddr_in servaddr;

    memset((void *) &servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT); /* numero di porta del server */

    serverPtr->serv_addr = servaddr;
}

void bind_address(int listen_sock, struct sockaddr_in serv_addr) {

    if ((bind(listen_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) == -1) {
        perror("errore in bind");
        exit(1);
    }
}

void init_pollfd(struct Server *serverPtr) {

    int i = 0;

    while (i < NUM_THREAD_POOL) {
        serverPtr->array_fd[i].fd = -1;
        serverPtr->array_fd[i].events = POLLIN | POLLPRI;
        i++;
    }

}

ServerPtr Server() {
    ServerPtr serverPtr = (ServerPtr) memory_alloc(sizeof(struct Server));
    serverPtr->listen_sock = create_socket();
    serverPtr->set_server_address = set_server_address;
    serverPtr->bind_address = bind_address;
    serverPtr->image_list = image_list;
    serverPtr->allocate_pool = allocate_pool;
    serverPtr->set_socket_options = set_socket_options;
    serverPtr->init_pollfd = init_pollfd;
    return serverPtr;
}

ServerPtr create_server() {

    ServerPtr serverPtr = Server();

    serverPtr->set_server_address(serverPtr);
    serverPtr->bind_address(serverPtr->listen_sock, serverPtr->serv_addr);
    serverPtr->image_list();

    if (listen(serverPtr->listen_sock, BACKLOG) < 0) {
        perror("Errore in listen");
        exit(EXIT_FAILURE);
    }

    serverPtr->set_socket_options(serverPtr->listen_sock, 0,
                                  SO_REUSEADDR);                                             /* It prevents error on bind() for subsequent run of the program */

    serverPtr->thread_pool = serverPtr->allocate_pool(
            NUM_THREAD_POOL);                                                 /* Preallocation of NUM_THREAD_POOL threads */

    serverPtr->init_pollfd;                                                                                             /* Initialization of the pool of fds that are going to be checked in poll()*/
    serverPtr->array_fd[0].fd = serverPtr->listen_sock;
    serverPtr->array_fd[0].events = POLLIN | POLLPRI;
    serverPtr->thread_pool->slot_used[0] = 1;

    serverPtr->log_fp = open_fp(SERVER_LOG_PATH, "a+");

    /*connectToDB();
    //prepareConnectionDB();
    init_pollfd();
    structpollfd[0].fd = serverPtr->listen_sock;
    structpollfd[0].events = POLLPRI | POLLIN;

    init_mutex(&image_mtx);
    int i = -1;
    while(++i < NUM_THREAD_POOL)
        init_cond(cond_msg_converted+i);

    init_buffer_image();*/

    return serverPtr;

}

