#include <sys/poll.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <ImageMagick-7/MagickWand/MagickWand.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#include "include/Config.h"
#include "include/HttpServer.h"
#include "include/ThreadPool.h"
#include "include/Utils.h"
#include "include/Log.h"
#include "include/Message.h"
#include "include/HandleDB.h"
#include "include/Strings.h"

struct thread_pool *thread_pool;
nfds_t max_descriptor;
char *file_map;
size_t seek_cache;
FILE *log_fp;
struct image_t icon;
struct pollfd array_fd[NUM_THREAD_POOL];

int main(int argc, char *argv[]) {

    MagickWandGenesis();                                                                                                /* Initializes the MagickWand environment */

    int conn_sd;                                                                                                        /* Connection's file descriptor */
    int ready;                                                                                                          /* Number of ready fd */
    int index;                                                                                                          /* Iterators */
    int ret;
    nfds_t d;

    max_descriptor = 1;                                                                                                 /* Number of descriptors to be checked */

    socklen_t socklen;
    struct sockaddr_in client_addr;

    ServerPtr serverPtr = create_server();
    thread_pool = serverPtr->thread_pool;
    struct thread_data *thread_data = thread_pool->td_pool;

    /*****************************    Main loop   ***********************************************************/
    while (1) {

        ready = poll(array_fd, max_descriptor, 1000);

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
                    printf("descriptor %d %d\n", index, array_fd[index++].fd);
                continue;

            default:
                break;
        }

        for (d = 0; d < max_descriptor; d++) {

            if (array_fd[d].revents &
                POLLIN) {                                                                                               /* If the d-ith descriptor has received bytes to be read */

                if (array_fd[d].fd ==
                    serverPtr->listen_sock) {                                                                           /* If the descriptor chosen is the listen socket, there is new connection */

                    socklen = sizeof(client_addr);
                    conn_sd = accept(serverPtr->listen_sock,
                                     (struct sockaddr *) &client_addr,
                                     &socklen);
                    abort_with_error("accept()", conn_sd == -1);

                    set_socket_options(conn_sd, SO_KEEPALIVE, 0);

                    /**************** Searching for a free slot *************************************/
                    get_mutex(&(thread_pool->mtx));

                    int nE = get_E(
                            thread_pool,
                            thread_pool->S,
                            thread_pool->E);                                                                            /* Finds a free slot in the ring buffer */
                    thread_pool->E = nE;
                    printf("%d\n", thread_pool->E);

                    thread_pool->td_pool[nE].conn_sd = conn_sd;                                      /* The rest of assignments are done in allocate_pool() */
                    thread_pool->td_pool[nE].E = nE;
                    thread_pool->td_pool[nE].client_addr = client_addr;
                    thread_pool->slot_used[nE] = 1;

                    release_mutex(&(thread_pool->mtx));
                    /******************************************************************************/

                    write_event_log(log_fp, CONNECTION_ACCEPTED,
                                    client_addr, NULL);                                                                 /* Keeps track of the client's connection in the log file */

                    ret = pthread_create(&(thread_data[nE].tid), NULL, handle_client, thread_data+nE);
                    if (ret) abort_with_error("pthread_create()\n", ret != 0);

                } else {                                                                                                /* If the invoked descriptor is not the listening socket, new data have been sent to old socket */

                    pthread_t tid;
                    int v;

                    get_mutex(&(thread_pool->mtx));
                    int E = find_E_for_fd(thread_pool, thread_pool->S, thread_pool->E, array_fd[d].fd);
                    release_mutex(&(thread_pool->mtx));

                    thread_data[E].msg_received = 1;                                                                    /* Utile per il timer */

                    printf("\n\nIN ELSE\n\n");

                    //v = pthread_create(&tid, NULL, handle_client, &(thread_data[E]));
                    //abort_with_error("pthread_create()", v != 0);
                    signal_cond(&(thread_data[E].cond_msg));

                    continue;

                }

            } else if (array_fd[d].revents & POLLPRI) {
                fprintf(stderr, "POLLPRI\n");
            }

        }

    }
    /**********************************************************************************************************/

    return EXIT_SUCCESS;
}

void get_image_to_send(struct image_t *image) {

    int fd;
    char *image_path = NULL;

    size_t width = 0, height = 0;

    if (image->image_name != NULL) {
        image_path = catenate_strings(IMAGE_DIR, image->image_name);
        image_path = catenate_strings(image_path, image->ext);
    }

    if(image->cached == CACHED_IMAGE) {
        fd = open_file(image->cache_path, O_RDONLY);
        if (fd != -1) {
            image->file_size = get_file_size(fd);
            image->fd = fd;
            return;
        }
    } else {
        fd = open_file(image->cache_path, O_CREAT | O_RDWR);
    }

    MagickWand *magickWand = NewMagickWand();

    MagickBooleanType result = MagickReadImage(magickWand, image_path);
    if(result == MagickFalse && !strcmp(image->ext, ".jpg")) {
        image_path = catenate_strings(IMAGE_DIR, image->image_name);
        image_path = catenate_strings(image_path, ".jpeg");
        result = MagickReadImage(magickWand, image_path);
        abort_with_error("MagickWriteImage()", result == MagickFalse);
    }

    width = image->width ? (size_t) image->width : MagickGetImageWidth(magickWand);
    height = image->height ? (size_t) image->height : MagickGetImageHeight(magickWand);

    MagickResetIterator(magickWand);
    while(MagickNextImage(magickWand) == MagickFalse) {
        MagickSetImageFormat(magickWand, image->ext);
        MagickSetCompressionQuality(magickWand, (size_t)(image->image_list[0].q * 100));
        MagickResizeImage(magickWand, width, height, LanczosFilter);
    }

    result = MagickWriteImage(magickWand, image->cache_path);
    abort_with_error("MagickWriteImage", result == MagickFalse);

    free(image_path);

    DestroyMagickWand(magickWand);

    image->file_size = get_file_size(fd);
    memcpy(file_map + seek_cache, image->cache_path, 128);
    seek_cache = (seek_cache + strlen(image->cache_path)) % SIZE_FILE_LISTCACHE;

    image->fd = open_file(image->cache_path, O_RDONLY);

}

void *handle_client(void *arg) {

    struct thread_data *td = (struct thread_data *) arg;                                                                /* Cast useful to retrieve struct thread_data's fields*/
    td->msg_received = 1;                                                                                               /* A message has been received */
    struct Request *request = create_request();
    struct image_t *image_info = memory_alloc(sizeof(struct image_t));

    int status_r, ret, v;
    max_descriptor++;
    array_fd[td->E].fd = td->conn_sd;

    while (1) {

        get_mutex(&(td->mtx_msg_socket));
        int idx = td->idx;
        td->idx = (td->idx + 1) % 5;


        td->message[idx] = (char *) memory_alloc(HTTP_MESSAGE_SIZE);

        status_r = receive_message(td, idx);
        release_mutex(&(td->mtx_msg_socket));

        switch (status_r) {

            case REQUEST_RECEIVED:                                                                                      /* A request has been sent by the client */

                ret = parse_message(td->message[idx], &request);
                if (ret == REQUEST_RECEIVED) {
                    printf("user agent: %s\n", request->user_agent);

                    retrieve_from_DB(request,
                                     td->connDB);                                                                           /* Here is set also the cache name */

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
                    if(image_info->image_name != NULL)
                        image_info->image_path = catenate_strings(IMAGE_DIR, image_info->image_name);

                    image_info->cached = find_file_in_cache(image_info->cache_path,
                                                            file_map);                                                  /* Scan cache of images. One I/O instead of two */

                    write_event_log(log_fp, LOG_IMAGE_REQUESTED,
                                    td->client_addr, image_info);

                    get_image_to_send(image_info);

                    v = send_image(td->conn_sd, image_info, request->cmd);

                } else if (ret == ICON_REQUESTED) {
                    icon.fd = open_file(ICON_PATH, O_RDONLY);
                    icon.file_size = get_file_size(icon.fd);
                    send_image(td->conn_sd, &icon, GET_CMD);
                }

                break;

            case EMPTY_MESSAGE:
                thread_pool->slot_used[td->E] = 0;
                max_descriptor--;

                free(request->ext);
                free(request);


                shutdown(td->conn_sd, SHUT_RDWR);
                close(td->conn_sd);
                pthread_exit(NULL);

            case MESSAGE_NOT_CORRECT:
                //deal with it

            default:
                break;
        }

        get_mutex(&(td->mtx_msg_socket));
        wait_cond(&(td->cond_msg), &(td->mtx_msg_socket));
        release_mutex(&(td->mtx_msg_socket));

        /************** After release condition *****************************/

        /*if (td->msg_received == 1)
            continue;
        else {
            //operazioni di uscita
            free(image_info);
            free(request);
            pthread_exit(NULL);
        }*/

        //get_mutex(&(thread_pool->mtx));
        //while (pool->S == pool->E)
        //    wait_for_free_slot(&(pool->cb_not_empty), &(pool->mtx));

        //thread_pool->S = (thread_pool->S + 1) % NUM_THREAD_POOL;
        //signal_cond(&(thread_pool->cb_not_full));

        //release_mutex(&(thread_pool->mtx));

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

void init_pollfd() {

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
    if(fd != -1) {
        abort_with_error("lseek()", lseek(fd, SIZE_FILE_LISTCACHE, SEEK_SET) == -1);                                    /* Makes a hole in the file */
        abort_with_error("write()", write(fd, &fd, 1) != 1);
    } else {
        fd = open_file(IMAGE_CACHE_FILE, O_RDWR);
    }

    char *map = mmap(NULL, SIZE_FILE_LISTCACHE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    abort_with_error("mmap()", map == MAP_FAILED);

    errno = 0;
    mlock(map, SIZE_FILE_LISTCACHE);                                                                                    /* Keeps file in memory in order to avoid I/O operations */
    printf("%s\n", strerror(errno));

    return map;

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
    //serverPtr->list_file_in_cache = list_file_in_cache;
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
                                  SO_REUSEADDR);                                                                        /* It prevents error on bind() for subsequent run of the program */

    set_number_of_connections();                                                                                        /* Allows one connection per thread */

    serverPtr->thread_pool = serverPtr->allocate_pool(
            NUM_THREAD_POOL);                                                                                           /* Preallocation of NUM_THREAD_POOL threads */

    serverPtr->init_pollfd();                                                                                             /* Initialization of the pool of fds that are going to be checked in poll()*/
    array_fd[0].fd = serverPtr->listen_sock;
    array_fd[0].events = POLLIN | POLLPRI;
    serverPtr->thread_pool->slot_used[0] = 1;

    log_fp = open_fp(SERVER_LOG_PATH, "a+");

    /* OPERATION NOT PERMITTED
struct rlimit rlim;
rlim.rlim_cur = 524288;
rlim.rlim_max = 524288;

errno = 0;
if(setrlimit(RLIMIT_MEMLOCK, &rlim) == -1)
    fprintf(stderr, "%s\n", strerror(errno));

getrlimit(RLIMIT_MEMLOCK, &rlim);
printf("max mem %ld %ld \n", rlim.rlim_cur, rlim.rlim_max);
    */

    file_map = get_cache_file();
    seek_cache = 0;

    /******************** Icon ****************************/
    icon.fd = open_file(ICON_PATH, O_RDONLY);
    icon.image_name = ICON_PATH;
    //icon.file_size = get_file_size(icon.fd);
    /*****************************************************/

    return serverPtr;
}

