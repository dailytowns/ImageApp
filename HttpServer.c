#include <sys/poll.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <ImageMagick-7/MagickWand/MagickWand.h>
#include <errno.h>
#include <signal.h>

#include "include/HttpServer.h"
#include "include/Utils.h"
#include "include/ThreadPool.h"
#include "include/HandleImage.h"
#include "include/Request.h"
#include "include/Log.h"
#include "include/HandleDB.h"

struct image_t icon;
char *file_map;
size_t seek_cache;

int main() {

    MagickWandGenesis();                                                                                                /* Initializes the MagickWand environment */

    int conn_sd;                                                                                                        /* Connection's file descriptor */
    int ready;                                                                                                          /* Number of ready fd */
    max_descriptor = 1;                                                                                                 /* Number of descriptors to be checked */

    socklen_t socklen;                                                                                                  /* Size of client's address*/
    struct sockaddr_in client_addr;                                                                                     /* Address of the client */

    struct server_t *server = init_server();                                                                            /* Initialize the main components of the server */
    int idx_pool = 0;                                                                                                   /* Index of the pool that is going to be used */

    struct pollfd *pfd = memory_alloc(sizeof(struct pollfd));                                                           /* Contains listen socket's file descriptor. It is used in poll() */
    pfd->events = POLLIN;
    pfd->fd = server->listen_sock;

    int counter = 0;                                                                                                    /* Used during tests */
    size_t s = size_pool;                                                                                               /* Number of pools in memory */

    /* Fork processes to dispatch */

    /************************************************ Main loop *******************************************************/

    while (1) {

        /************************* Poll descriptors ********************************************/
        ready = poll(pfd, 1, 1000);
        if (ready ==
            -1) {                                                                                                       /* If an error occurs in poll() */
            fprintf(stderr, "Error in poll()\n");
            if (errno == ENOMEM) {
                fprintf(stderr, "in select, errno %d\n", errno);
                exit(EXIT_FAILURE);                                                                                     /* If poll() fails, the program cannot execute correctly*/
            }
        } else if (ready == 0) {
            continue;
        }
        /*************************************************************************************/

        if ((pfd->revents &
             POLLIN) !=
            0) {                                                                                                        /* If this descriptor is ready to receive bytes */
            socklen = sizeof(client_addr);

            conn_sd = accept(server->listen_sock,
                             (struct sockaddr *) &client_addr,
                             &socklen);
            abort_with_error("accept()", conn_sd == -1);

            //fprintf(stderr, "counter %d\n",
            //        ++counter);                                                                                       /* Used during test */

            write_event_log(log_fp, CONNECTION_ACCEPTED,
                            client_addr,
                            NULL);                                                                                      /* Keeps track of the client's connection in the log file */

            get_mutex(&mtx_max_d);
                max_descriptor++;
            release_mutex(&mtx_max_d);

            (dispatcher + idx_pool)->conn_sd = conn_sd;
            (dispatcher + idx_pool)->client_addr = client_addr;

            get_mutex(&(dispatcher + idx_pool)->mtx_dispatch);
            signal_cond(&((dispatcher +
                           idx_pool)->cond_dispatch));                                                                  /* The mutexes are added to prevent the loss of a signal on the condition variable */
            release_mutex(&(dispatcher + idx_pool)->mtx_dispatch);

            if (max_descriptor >= ((num_thread_pool * s) / 2)) {

                if ((s + s) < 32) {

                    fprintf(stderr, "\n\nREALLOCING, size_pool %ld\n\n", s);

                    /****************** Number of pools growing ****************************/

                    signal_cond(&cond_realloc);

                    get_mutex(&mtx_realloc_end);
                        wait_cond(&cond_realloc_end, &mtx_realloc_end);
                    release_mutex(&mtx_realloc_end);

                }
                /***********************************************************************/

                idx_pool = (int) (num_thread_pool * s);

                s = size_pool;
            }

            idx_pool = (idx_pool + 1) % s;
        }

    }
    /******************************************************************************************************************/

}



void *handle_client(void *arg) {

    struct thread_data *td = (struct thread_data *) arg;
    struct request_t *request = NULL;
    struct image_t *image_info = NULL;
    struct pollfd *pfd = memory_alloc(sizeof(struct pollfd));

    /**
     * These functions were commented because during the tests the program
     * crashes fewer times on average
     *
     * long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
     * set_thread_affinity(td->E % (int)num_cores);
     */

    int status_r, ret, v, timer;
    int idx_pool = td->idx_pool;
    int E = td->E;
    int idx = td->idx;
    int idx_fd_cache;

    while (1) {

        /* If no request is present, wait for a signal */
        wait_for_connection(td);

        idx = (idx + 1) % 5;                                                                                            /* This index was aimed to implement pipelined requests */

        status_r = receive_request(td, idx);

        if (td->request ==
            0) {                                                                                                        /* If the client ran out requests, it is warned and connection is closed */
            send_service_unavailable(td->conn_sd);
            status_r = EMPTY_MESSAGE;
        }

        switch (status_r) {

            case REQUEST_RECEIVED:

                ret = parse_request(td->message[idx], &request);

                if (ret == REQUEST_RECEIVED) {
                    retrieve_dim_from_DB(request, td->connDB);                                                          /* Here is set also the cache name */

                    image_info = memory_alloc(sizeof(struct image_t));

                    /** Retrieved informations from the db, there are all the parameters needed to send the correct image **/
                    image_info->width = request->width;
                    image_info->height = request->height;
                    image_info->image_name = request->image_name;
                    image_info->image_list = request->image_list;
                    image_info->cache_name = request->cache_name;
                    image_info->ext = request->ext;
                    image_info->colors = request->colors;

                    if (image_info->cache_name != NULL) {
                        image_info->cache_path = memory_alloc(strlen(IMAGE_CACHE) + strlen(image_info->cache_name) + strlen(image_info->ext) + 1);
                        sprintf(image_info->cache_path, "%s%s%s", IMAGE_CACHE, image_info->cache_name, image_info->ext);
                    }

                    if (image_info->image_name != NULL) {
                        image_info->image_path = memory_alloc(strlen(IMAGE_DIR) + strlen(image_info->image_name) + strlen(image_info->ext) + 1);
                        sprintf(image_info->image_path, "%s%s%s", IMAGE_DIR, image_info->image_name, image_info->ext);
                    }

                    image_info->cached = find_file_in_cache(image_info->cache_path,
                                                            file_map,
                                                            &idx_fd_cache);                                                  /* Scan cache of images */
                    write_event_log(log_fp, LOG_IMAGE_REQUESTED,
                                    td->client_addr, image_info);                                                       /* Keeps track of which image the client requested */


                    /**************************************************************************************************/

                    ret = get_image_to_send(image_info, idx_fd_cache);

                    if (ret == IMAGE_NOT_PRESENT) {

                        send_bad_request(td->conn_sd);

                        get_mutex(&mtx_max_d);
                        max_descriptor--;
                        release_mutex(&mtx_max_d);

                        write_event_log(log_fp, LOG_IMAGE_NOT_PRESENT,
                                        td->client_addr,
                                        image_info);                                                                    /* Keeps track of the client's connection in the log file */

                        shutdown(td->conn_sd, SHUT_RDWR);
                        close(td->conn_sd);

                        get_mutex(&(*(pool + idx_pool))->mtx);
                            signal_cond(&(*(pool + idx_pool))->cb_not_full);
                        release_mutex(&(*(pool + idx_pool))->mtx);

                    } else {                                                                                            /* Parsing OK */

                        v = send_image(((*(pool + idx_pool))->arr + E)->conn_sd, image_info, request->cmd);
                        //close(image_info->fd);

                        /* If client has disconnected before having received the response */
                        if (v == CONNECTION_CLOSED) {

                            close_client_with_status(td, idx_pool, CONNECTION_CLOSED);
                            pfd->fd = -1;
                            destroy_request(request, CONNECTION_CLOSED);
                            destroy_image(image_info);
                            bzero(td->message[idx], HTTP_MESSAGE_SIZE);
                            continue;

                        }
                    }

                } else if (ret == HEAD_CMD) {
                    send_image(td->conn_sd, NULL, HEAD_CMD);
                } else if (ret == ICON_REQUESTED) {
                    send_image(td->conn_sd, &icon, GET_CMD);
                } else if (ret == MESSAGE_NOT_CORRECT) {
                    send_bad_request(td->conn_sd);
                }

                /*********** Updates the array of descriptors to received more data ***************************/
                td->msg_received = 0;
                bzero(td->message[idx], HTTP_MESSAGE_SIZE);
                /********************************************************************************************/
                break;

            case EMPTY_MESSAGE:                                                                                         /* Client disconnected */

                close_client_with_status(td, idx_pool, CONNECTION_CLOSED);
                pfd->fd = -1;
                destroy_request(request, CONNECTION_CLOSED);
                destroy_image(image_info);
                continue;

            default:
                break;
        }

        pfd->fd = td->conn_sd;
        pfd->events = POLLIN;

        timer = handle_timer(idx_pool, E, pfd);
        if (timer == 0) {

            close_client_with_status(td, idx_pool, CONNECTION_CLOSED);
            destroy_request(request, status_r);
            destroy_image(image_info);

        }

    }

}

struct server_t *
Server() {

    struct server_t *serverPtr = memory_alloc(
            sizeof(struct server_t));                                                                                   /* Allocates memory for the server */
    return serverPtr;

}

void sig_hndl(int sig) {

    int i = 0, j=0;

    while(i < size_pool) {

        j = 0;
        while(j < num_thread_pool) {
            fprintf(stderr, "pool %d thread %d used %d\n", i, j, pool[i]->arr[j].used);
            j++;
        }

        i++;
    }

}

struct server_t *init_server() {

    struct server_t *serverPtr = Server();

    parse_config_file();

    /*************************** Creating listen socket ************************/

    serverPtr->listen_sock = create_socket();

    /* It prevents error on bind() for subsequent run of the program */
    set_socket_options(serverPtr->listen_sock, 0,
                       SO_REUSEADDR);

    struct sockaddr_in servaddr;

    memset((void *) &servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    servaddr.sin_port = htons(
            (uint16_t) serv_port);                                                                                      /* Port number chosen in serv.conf file */
    serverPtr->serv_addr = servaddr;

    bind_address(serverPtr->listen_sock, serverPtr->serv_addr);

    if (listen(serverPtr->listen_sock, backlog) < 0) {
        perror("Errore in listen");
        exit(EXIT_FAILURE);
    }

    /*************************************************************************/

    image_list();
    log_fp = get_log();

    set_number_of_connections();                                                                                        /* Allows one connection per thread */

    pool = memory_alloc(32 * sizeof(struct pool_t *));

    int i = 0;
    while (i < size_pool) {

        allocate_pool(
                num_thread_pool, pool,
                i);
        i++;
    }

    log_fp = get_log();

    file_map = get_cache_file();                                                                                        /* Creates the file that contains name of the images resized previously */
    seek_cache = 0;                                                                                                     /* Seek in the cache file */

    /******************** Icon ****************************/
    icon.fd = open_file(ICON_PATH,
                        O_RDONLY);                                                                                      /* The icon is opened once and for all because it is requested with high frequency */
    icon.image_name = ICON_PATH;
    icon.file_size = get_file_size(icon.fd);
    /*****************************************************/

    init_mutex(&mtx_accept);
    init_mutex(&mtx_realloc_end);
    init_cond(&cond_realloc);
    init_cond(&cond_realloc_end);
    init_mutex(&mtx_realloc);
    init_mutex(&mtx_max_d);

    init_mtx_db();

    dispatcher = memory_alloc(32 * sizeof(struct dispatcher));

    int counter = 0;
    while (counter < size_pool) {

        int *j = memory_alloc(sizeof(int));
        pthread_t tid;
        fprintf(stderr, "counter %d\n", counter);
        init_mutex(&((dispatcher + counter)->mtx_dispatch));
        init_cond(&((dispatcher + counter)->cond_dispatch));
        (dispatcher + counter)->idx_pool = counter;
        (dispatcher + counter)->request = 0;
        *j = counter;
        abort_with_error("pthread_create", pthread_create(&tid, NULL, dispatch_job, j) != 0);
        counter++;

    }

    pthread_t tid;
    abort_with_error("pthread_create", pthread_create(&tid, NULL, handle_pool, NULL) != 0);

    init_mutex(&fd_cache_mtx);
    fd_image_cache.E = 0;

    abort_with_error("signal", signal(SIGUSR1, sig_hndl) == SIG_ERR);


    return serverPtr;

}