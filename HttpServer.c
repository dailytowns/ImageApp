//
// Created by federico on 31/10/17.
//

#include <sys/poll.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <ImageMagick-7/MagickWand/MagickWand.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#include "include/HttpServer.h"
#include "include/Utils.h"
#include "include/ThreadPool.h"
#include "include/HandleImage.h"
#include "include/Request.h"
#include "include/Log.h"
#include "include/Strings.h"
#include "include/HandleDB.h"

FILE *log_fp;
struct image_t icon;
char *file_map;
size_t seek_cache;
struct pool_t *pool;
nfds_t max_descriptor;

int main() {

    MagickWandGenesis();                                                                                                /* Initializes the MagickWand environment */

    int conn_sd;                                                                                                        /* Connection's file descriptor */
    int ready;                                                                                                          /* Number of ready fd */
    int ret;
    int i, nE;
    nfds_t d;                                                                                                           /* Iterator */
    max_descriptor = 1;                                                                                          /* Number of descriptors to be checked */

    socklen_t socklen;
    struct sockaddr_in client_addr;

    struct server_t *server = init_server();
    pool = server->pool;
    struct thread_data *td = pool->arr;
    struct pollfd *arrfd = pool->array_fd;

    while (1) {

        /************************* Poll descriptors ********************************************/
        ready = poll(arrfd, max_descriptor, 1000);
        if (ready ==
            -1) {                                                                                              /* If an error occurs in poll() */
            fprintf(stderr, "Error in poll()\n");
            if (errno == EBADF || errno == ENOMEM) {
                fprintf(stderr, "in select, errno %d\n", errno);
                exit(EXIT_FAILURE);
            }
        } else if (ready == 0) {
            int fd = 0;
            while (fd < max_descriptor) {
                printf("arr fd 0 1 %d %d\n", fd, arrfd[fd].fd);
                fd++;
            }
            continue;
        }
        /*************************************************************************************/

        d = 0;
        i = 0;

        while (d <
               max_descriptor) {                                                                                     /* Iterate over d ready descriptors */

            if (arrfd[i].fd != -1) {

                if (arrfd[i].revents & POLLIN) {

                    if (arrfd[i].fd == server->listen_sock) {

                        socklen = sizeof(client_addr);

                        conn_sd = accept(server->listen_sock,
                                         (struct sockaddr *) &client_addr,
                                         &socklen);
                        abort_with_error("accept()", conn_sd == -1);

                        printf("%d connsd\n", conn_sd);

                        set_socket_options(conn_sd, SO_KEEPALIVE, 0);

                        /**************** Searching for a free slot *************************************/
                        get_mutex(&(pool->mtx));

                        nE = get_E(
                                pool);                                                                               /* Finds a free slot in the ring buffer */

                        pool->arr[nE].conn_sd = conn_sd;                                                                /* The rest of assignments are done in allocate_pool() */
                        pool->arr[nE].E = nE;
                        pool->arr[nE].client_addr = client_addr;
                        pool->arr[nE].timer = 5;
                        pool->arr[nE].request = 10;

                        release_mutex(&(pool->mtx));
                        /******************************************************************************/

                        write_event_log(log_fp, CONNECTION_ACCEPTED,
                                        client_addr,
                                        NULL);                                                                          /* Keeps track of the client's connection in the log file */

                    } else {
                        get_mutex(&(pool->mtx));
                        nE = find_E_for_fd(pool, arrfd[i].fd);
                        release_mutex(&(pool->mtx));

                        printf("IN ELSE\n");

                        if (nE != -1) {
                            get_mutex(&(pool->arr[nE].mtx_new_request));
                                pool->arr[nE].msg_received = 1;                                                         /* Used for the timer */
                            release_mutex(&(pool->arr[nE].mtx_new_request));
                        }
                    }

                    signal_cond(&(pool->arr[nE].cond_msg));
                    d++;
                    continue;

                } else if (arrfd[i].revents & POLLPRI) {
                    fprintf(stderr, "POLLPRI\n");
                }

            }
            i++;

        }

    }

}

void *handle_client(void *arg) {

    struct thread_data *td = (struct thread_data *) arg;
    struct request_t *request = create_request();
    struct image_t *image_info = memory_alloc(sizeof(struct image_t));

    int status_r, ret, v, timer;

    while (1) {

        get_mutex(&(td->mtx_new_request));
        printf("prima di cond\n");
        wait_cond(&td->cond_msg,
                  &td->mtx_new_request);                                                                                /* A message has been received */
        td->msg_received = 1;
        max_descriptor++;

        int idx = td->idx;
        td->idx = (td->idx + 1) % 5;

        status_r = receive_request(td, idx);
        release_mutex(&(td->mtx_new_request));

        switch (status_r) {

            case REQUEST_RECEIVED:
                ret = parse_request(td->message[idx], &request);
                if (ret == REQUEST_RECEIVED) {
                    retrieve_from_DB(request,
                                     td->connDB);                                                                       /* Here is set also the cache name */

                    image_info->width = request->width;
                    image_info->height = request->height;
                    image_info->image_name = request->image_name;
                    image_info->image_list = request->image_list;
                    image_info->cache_name = request->cache_name;
                    image_info->ext = request->ext;

                    if (image_info->cache_name != NULL) {
                        image_info->cache_path = catenate_strings(IMAGE_CACHE, image_info->cache_name);
                        image_info->cache_path = catenate_strings(image_info->cache_path, image_info->ext);
                    }
                    if (image_info->image_name != NULL)
                        image_info->image_path = catenate_strings(IMAGE_DIR, image_info->image_name);

                    image_info->cached = find_file_in_cache(image_info->cache_path,
                                                            file_map);                                                  /* Scan cache of images. One I/O instead of two */

                    write_event_log(log_fp, LOG_IMAGE_REQUESTED,
                                    td->client_addr, image_info);

                    ret = get_image_to_send(image_info);
                    if (ret == IMAGE_NOT_PRESENT) {
                        send_bad_request(td->conn_sd);
                        pool->array_fd[td->E].fd = -1;
                        max_descriptor--;

                        free(request->ext);
                        free(request);

                        shutdown(td->conn_sd, SHUT_RDWR);
                        close(td->conn_sd);
                    } else {
                        v = send_image(td->conn_sd, image_info, request->cmd);
                        if (v == CONNECTION_CLOSED) {
                            get_mutex(&(td->mtx_new_request));
                                td->msg_received = 0;
                                max_descriptor--;
                                pool->array_fd[td->E].fd = -1;
                                shutdown(td->conn_sd, SHUT_RDWR);
                                close(td->conn_sd);

                            get_mutex(&(pool->mtx));
                                signal_cond(&pool->cb_not_full);
                            release_mutex(&(pool->mtx));

                            release_mutex(&(td->mtx_new_request));
                        }
                    }

                } else if (ret == HEAD_CMD) {
                    send_image(td->conn_sd, NULL, HEAD_CMD);
                } else if (ret == ICON_REQUESTED) {
                    send_image(td->conn_sd, &icon, GET_CMD);
                } else if (ret == MESSAGE_NOT_CORRECT) {
                    send_bad_request(td->conn_sd);
                }

                get_mutex(&(td->mtx_new_request));
                td->msg_received = 0;
                //max_descriptor++;
                pool->array_fd[td->E].fd = td->conn_sd;
                release_mutex(&(td->mtx_new_request));

            case EMPTY_MESSAGE:
                get_mutex(&(td->mtx_new_request));
                    td->msg_received = 0;
                    max_descriptor--;
                    pool->array_fd[td->E].fd = -1;
                    shutdown(td->conn_sd, SHUT_RDWR);
                    if(close(td->conn_sd) == -1){
                        fprintf(stderr, "Error in close(), errno %d\n strerror %s\n", errno, strerror(errno));
                    };
                signal_cond(&(pool->cb_not_full));
                release_mutex(&(td->mtx_new_request));
                continue;

            default:
                break;
        }

        timer = handle_timer(td);
        if (timer == 0) {
            get_mutex(&(td->mtx_new_request));
                max_descriptor--;
                pool->array_fd[td->E].fd = -1;
                shutdown(td->conn_sd, SHUT_RDWR);
                close(td->conn_sd);
                td->msg_received = 0;
                signal_cond(&pool->cb_not_full);

            release_mutex(&(td->mtx_new_request));

            printf("dopo timer\n");
        } else {
            signal_cond(&td->cond_msg);
            continue;
        }
    }

}

int create_socket() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        fprintf(stderr, "Error creating the socket\n");
        exit(EXIT_FAILURE);
    }
    return sock_fd;
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
        if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) == -1) {
            fprintf(stderr, "Error in setsockopt()\n");
            exit(EXIT_FAILURE);
        }
    }

}

void set_server_address(struct server_t *serverPtr) {

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

void init_pollfd(struct pollfd *array_fd) {

    int i = 0;

    while (i < NUM_THREAD_POOL) {
        array_fd[i].fd = -1;
        array_fd[i].events = POLLIN | POLLPRI;
        i++;
    }

}

char *get_cache_file() {

    int fd;

    fd = open_file(IMAGE_CACHE_FILE, O_CREAT | O_EXCL | O_RDWR);
    if (fd != -1) {
        abort_with_error("lseek()", lseek(fd, SIZE_FILE_LISTCACHE, SEEK_SET) ==
                                    -1);                                    /* Makes a hole in the file */
        abort_with_error("write()", write(fd, &fd, 1) != 1);
    } else {
        fd = open_file(IMAGE_CACHE_FILE, O_RDWR);
    }

    char *map = mmap(NULL, SIZE_FILE_LISTCACHE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    abort_with_error("mmap()", map == MAP_FAILED);

    errno = 0;
    mlock(map,
          SIZE_FILE_LISTCACHE);                                                                                    /* Keeps file in memory in order to avoid I/O operations */
    printf("%s\n", strerror(errno));

    return map;

}

struct server_t *
Server() {                                                                                             /* Allocates memory for the server */
    struct server_t *serverPtr = (struct server_t *) memory_alloc(sizeof(struct server_t));
    return serverPtr;
}

struct server_t *init_server() {

    struct server_t *serverPtr = Server();

    serverPtr->listen_sock = create_socket();
    set_server_address(serverPtr);
    bind_address(serverPtr->listen_sock, serverPtr->serv_addr);
    image_list();

    if (listen(serverPtr->listen_sock, BACKLOG) < 0) {
        perror("Errore in listen");
        exit(EXIT_FAILURE);
    }

    set_socket_options(serverPtr->listen_sock, 0,
                       SO_REUSEADDR);                                                                        /* It prevents error on bind() for subsequent run of the program */

    set_number_of_connections();                                                                                        /* Allows one connection per thread */

    serverPtr->pool = allocate_pool(
            NUM_THREAD_POOL);                                                                   /* Preallocation of NUM_THREAD_POOL threads */

    init_pollfd(
            serverPtr->pool->array_fd);                                                                             /* Initialization of the pool of fds that are going to be checked in poll()*/
    serverPtr->pool->array_fd[0].fd = serverPtr->listen_sock;
    serverPtr->pool->array_fd[0].events = POLLIN | POLLPRI;

    log_fp = open_fp(SERVER_LOG_PATH, "a+");

    file_map = get_cache_file();
    seek_cache = 0;

    /******************** Icon ****************************/
    icon.fd = open_file(ICON_PATH, O_RDONLY);
    icon.image_name = ICON_PATH;
    icon.file_size = get_file_size(icon.fd);
    /*****************************************************/



    return serverPtr;
}