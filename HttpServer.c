//
// Created by federico on 31/10/17.
//

#define _GNU_SOURCE

#include <sys/poll.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <ImageMagick-7/MagickWand/MagickWand.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#include <sched.h>
#include <time.h>
#include <assert.h>                                                                                                     /* man example */

#include "include/HttpServer.h"
#include "include/Utils.h"
#include "include/ThreadPool.h"
#include "include/HandleImage.h"
#include "include/Request.h"
#include "include/Log.h"
#include "include/HandleDB.h"

FILE *log_fp;
struct image_t icon;
char *file_map;
size_t seek_cache;
struct pool_t *pool;
nfds_t max_descriptor;
struct pollfd *arrfd;
//struct client_t arrclient[64];

int main() {

    MagickWandGenesis();                                                                                                /* Initializes the MagickWand environment */

    int conn_sd;                                                                                                        /* Connection's file descriptor */
    int ready;                                                                                                          /* Number of ready fd */
    int i = 0, j = 0, nE;                                                                                                          /* Index to iterate over the file descriptors array */
    nfds_t d;                                                                                                           /* Iterator */
    max_descriptor = 1;                                                                                                 /* Number of descriptors to be checked */

    socklen_t socklen;
    struct sockaddr_in client_addr;                                                                                     /* Address of the client */
    size_t size_pool = 4;

    struct server_t *server = init_server();                                                                            /* Initialize the main components of the server */
    int idx_pool = 0, idx, idx_arrfd;
    //pool = server->pool;
    //struct thread_data *td = pool->arr;
    //arrfd = pool->array_fd;                                                                                           /* Avoids double reference*/

    while (1) {

        /************************* Poll descriptors ********************************************/
        ready = poll(arrfd, max_descriptor, 1000);
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

        d = 0;
        i = 0;

        while ((d < max_descriptor) && (i < num_thread_pool)) {                                                                                        /* Iterate over d ready descriptors */

            if (arrfd[i].fd !=
                -1) {                                                                                                   /* If the descriptor has been set */
                if (arrfd[i].revents &
                    POLLIN) {                                                                                           /* If this is the descriptor that is ready to receive bytes */
                    if (arrfd[i].fd ==
                        server->listen_sock) {                                                                          /* If the descriptor ready to receive is the listen socket, it has to be created a new connection */

                        socklen = sizeof(client_addr);

                        conn_sd = accept(server->listen_sock,
                                         (struct sockaddr *) &client_addr,
                                         &socklen);
                        abort_with_error("accept()", conn_sd == -1);

                        set_socket_options(conn_sd, SO_KEEPALIVE, 0);

                        /**************** Searching for a free slot *************************************/
                        get_mutex(&(pool[idx_pool].mtx));

                            nE = get_E(
                                    &pool[idx_pool]);                                                                                  /* Finds a free slot in the ring buffer */
                            idx_arrfd = get_free_slot_arrfd(arrfd);

                            /***** Save informations associated with thread ******/
                            pool[idx_pool].arr[nE].conn_sd = conn_sd;
                            pool[idx_pool].arr[nE].E = nE;
                            pool[idx_pool].arr[nE].idx_arrfd = idx_arrfd;
                            pool[idx_pool].arr[nE].client_addr = client_addr;
                            pool[idx_pool].arr[nE].timer = 5;
                            pool[idx_pool].arr[nE].request = 10;
                            pool[idx_pool].arr[nE].idx_pool = idx_pool;
                            pool[idx_pool].arr[nE].msg_received = 1;
                            pool[idx_pool].arr[nE].used = 1;

                            //arrclient[nE].conn_sd = conn_sd;
                            //arrclient[nE].port = ntohs(client_addr.sin_port);
                            //pool->arr[nE].idx_pool = idx_pool;

                        printf("connsd %d port %d nE %d idx_pool %d\n", conn_sd, ntohs(client_addr.sin_port), nE, idx_pool);

                        release_mutex(&(pool[idx_pool].mtx));
                        /******************************************************************************/

                        write_event_log(log_fp, CONNECTION_ACCEPTED,
                                        client_addr,
                                        NULL);                                                                          /* Keeps track of the client's connection in the log file */

                        idx = idx_pool;
                        idx_pool = (idx_pool + 1) % 4;

                    } else {

                        int t = 0, ret = -1;

                        while(t < 4) {
                            get_mutex(&(pool[t].mtx));
                            ret = find_E_for_fd(pool+t, arrfd[i].fd);
                            release_mutex(&(pool[t].mtx));

                            if(ret != -1)
                                break;

                            t++;
                        }

                        printf("in else, found nE = %d\n", ret);

                        if (ret != -1) {
                            get_mutex(&(pool[t].arr[ret].mtx_new_request));
                                pool[t].arr[ret].msg_received = 1;                                                         /* Used for the timer */
                            release_mutex(&(pool[t].arr[ret].mtx_new_request));
                        }

                        arrfd[i].fd = -1;
                        nE = ret;
                        idx = t;
                    }

                    //get_mutex(&(pool[idx_pool].arr[nE].mtx_new_request));
                    //wait_cond(&(pool[idx_pool].arr[nE].cond_timer), &(pool[idx_pool].arr[nE].mtx_new_request));
                    signal_cond(&(pool[idx].arr[nE].cond_msg));
                    //release_mutex(&(pool[idx_pool].arr[nE].mtx_new_request));

                    d++;
                    continue;

                }

            }
            i++;

        }

        if (max_descriptor > (num_thread_pool-1)*size_pool) {
            //signal_cond();
        }

    }

}

int set_thread_affinity(int core_id) {                                                                                  /* man example */

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_t current_thread = pthread_self();
    return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

/*void *check_job(void *arg) {

    struct pollfd *pfd;
    struct thread_data *td;

    int i;

    while (1) {

        i = 0;

        if (pthread_mutex_lock(&(pool->mtx_pool)) != 0) {
            fprintf(stderr, "Error in pthread_mutex_trylock()\n");
            exit(EXIT_FAILURE);
        }
        wait_cond(&(pool->cond_pool), &(pool->mtx_pool));

        if ((max_descriptor >= num_thread_pool / 2) &&
            (num_thread_pool < 256)) {

            pfd = realloc(pool->array_fd, (num_thread_pool + num_thread_pool) * sizeof(struct pollfd));
            abort_with_error("realloc()", pfd == NULL);

            td = realloc(pool->arr, (num_thread_pool + num_thread_pool) * (sizeof(struct thread_data)));
            abort_with_error("realloc()", td == NULL);

            while (i < num_thread_pool) {

                pthread_t tid = 0;

                pool->array_fd[(num_thread_pool) + i].fd = -1;
                pool->array_fd[num_thread_pool + i].events = POLLIN;

                (pool->arr[num_thread_pool + i]).message = (char **) memory_alloc(
                        5 *
                        sizeof(char *));                                                                                    /* Allocated five slots for messages */
  /*              int msg = 0;
                while (msg < 5) {
                    (pool->arr[num_thread_pool + i]).message[msg] = memory_alloc(
                            HTTP_MESSAGE_SIZE);                                            /* Slots of 512 bytes */
    /*                msg++;
                }
                (pool->arr[num_thread_pool + i]).tid = tid;

                printf("i db %d\n", i);

                (pool->arr[num_thread_pool +
                           i]).connDB = connect_DB();                                                                         /* Every thread gets its connection to the db */

      /*          init_mutex(&((pool->arr[num_thread_pool + i]).mtx_msg_socket));
                init_mutex(&((pool->arr[num_thread_pool + i]).mtx_new_request));
                init_cond(&((pool->arr[num_thread_pool + i]).cond_msg));

                abort_with_error("pthread_create()",
                                 pthread_create(&((pool->arr[num_thread_pool + i]).tid),
                                                NULL,
                                                handle_client,
                                                (pool->arr + (num_thread_pool + i))) != 0);
                i++;
            }

            num_thread_pool += num_thread_pool;

            pool->array_fd = pfd;
            pool->arr = td;

            signal_cond(&pool->cond_pool_doubled);

        } else {
            usleep(50000);
        }

        release_mutex(&(pool->mtx_pool));

    }

}*/

void *handle_client(void *arg) {

    struct thread_data *td = (struct thread_data *) arg;
    struct request_t *request = create_request();
    struct image_t *image_info = memory_alloc(sizeof(struct image_t));

    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    set_thread_affinity(td->E % (int)num_cores);

    int status_r, ret, v, timer;

    while (1) {

        get_mutex(&(td->mtx_new_request));

        if(td->msg_received != 1)
            wait_cond(&td->cond_msg,
                      &td->mtx_new_request);                                                                                /* A message has been received */

        printf("dopo wait\n");

        max_descriptor++;

        int idx = td->idx;
        td->idx = (td->idx + 1) % 5;

        status_r = receive_request(td, idx);

        arrfd[td->idx_arrfd].fd = td->conn_sd;

        if(td->request == 0) {
            send_service_unavailable(td->conn_sd);
            status_r = EMPTY_MESSAGE;
        }

        release_mutex(&(td->mtx_new_request));

        switch (status_r) {

            case REQUEST_RECEIVED:
                ret = parse_request(td->message[idx], &request);
                if (ret == REQUEST_RECEIVED) {
                    retrieve_dim_from_DB(request,
                                         td->connDB);                                                                   /* Here is set also the cache name */

                    image_info->width = request->width;
                    image_info->height = request->height;
                    image_info->image_name = request->image_name;
                    image_info->image_list = request->image_list;
                    image_info->cache_name = request->cache_name;
                    image_info->ext = request->ext;
                    image_info->colors = request->colors;

                    if (image_info->cache_name != NULL) {
                        image_info->cache_path = catenate_strings(IMAGE_CACHE, image_info->cache_name);
                        image_info->cache_path = catenate_strings(image_info->cache_path, image_info->ext);
                    }
                    if (image_info->image_name != NULL) {
                        image_info->image_path = catenate_strings(IMAGE_DIR, image_info->image_name);
                        image_info->image_path = catenate_strings(image_info->image_path, image_info->ext);
                    }

                    image_info->cached = find_file_in_cache(image_info->cache_path,
                                                            file_map);                                                  /* Scan cache of images. One I/O instead of two */

                    write_event_log(log_fp, LOG_IMAGE_REQUESTED,
                                    td->client_addr, image_info);

                    ret = get_image_to_send(image_info);

                    if (ret == IMAGE_NOT_PRESENT) {
                        send_bad_request(td->conn_sd);
                        get_mutex(&(td->mtx_new_request));
                            arrfd[td->idx_arrfd].fd = -1;
                            //arrclient[td->E].conn_sd = -1;
                            //arrclient[td->E].port = -1;
                            max_descriptor--;

                            write_event_log(log_fp, LOG_IMAGE_NOT_PRESENT,
                                            td->client_addr,
                                            NULL);                                                                      /* Keeps track of the client's connection in the log file */

                            free(request->ext);
                            free(request);

                            shutdown(td->conn_sd, SHUT_RDWR);
                            close(td->conn_sd);
                        release_mutex(&(td->mtx_new_request));
                    } else {
                        v = send_image(td->conn_sd, image_info, request->cmd);
                        if (v == CONNECTION_CLOSED) {
                            get_mutex(&(td->mtx_new_request));                                                          /* Avoids that a new client has its fd erased just after its creation */
                                td->msg_received = 0;
                                max_descriptor--;
                                arrfd[td->idx_arrfd].fd = -1;
                                //arrclient[td->E].conn_sd = -1;
                                //arrclient[td->E].port = -1;
                                shutdown(td->conn_sd, SHUT_RDWR);
                                close(td->conn_sd);

                            get_mutex(&(pool[td->idx_pool].mtx));
                                signal_cond(&pool[td->idx_pool].cb_not_full);
                            release_mutex(&(pool[td->idx_pool].mtx));

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

                /*********** Updates the array of descriptors to received more data ***************************/
                get_mutex(&(td->mtx_new_request));
                td->msg_received = 0;
                release_mutex(&(td->mtx_new_request));
                /********************************************************************************************/
                break;

            case EMPTY_MESSAGE:
                get_mutex(&(td->mtx_new_request));
                    td->msg_received = 0;
                    max_descriptor--;
                    arrfd[td->idx_arrfd].fd = -1;
                    td->used = -1;
                    //arrclient[td->E].conn_sd = -1;
                    //arrclient[td->E].port = -1;

                printf("in empty message\n");

                    write_event_log(log_fp, CONNECTION_CLOSED,
                                td->client_addr,
                                NULL);                                                                                  /* Keeps track of the client's connection in the log file */

                    shutdown(td->conn_sd, SHUT_RDWR);
                    if(close(td->conn_sd) == -1){
                        fprintf(stderr, "Error in close(), errno %d\n strerror %s\n", errno, strerror(errno));
                    };
                signal_cond(&(pool[td->idx_pool].cb_not_full));
                release_mutex(&(td->mtx_new_request));
                continue;

            default:
                break;
        }

        timer = handle_timer(td);
        if (timer == 0) {
            get_mutex(&(td->mtx_new_request));

                max_descriptor--;
                arrfd[td->idx_arrfd].fd = -1;
                shutdown(td->conn_sd, SHUT_RDWR);
                close(td->conn_sd);
                td->msg_received = 0;
                td->timer = 5;
                td->request = 10;
                td->used = -1;

                signal_cond(&pool[td->idx_pool].cb_not_full);

            printf("in finish timer, td->E %d td->idx_pool %d\n", td->E, td->idx_pool);

            release_mutex(&(td->mtx_new_request));
        } else {
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

    servaddr.sin_port = htons((uint16_t)serv_port);                                                                               /* numero di porta del server */
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

    while (i < num_thread_pool) {
        array_fd[i].fd = -1;
        array_fd[i].events = POLLIN;
        i++;
    }

}

char *get_cache_file() {

    int fd;

    fd = open_file(IMAGE_CACHE_FILE, O_CREAT | O_EXCL | O_RDWR);
    if (fd != -1) {
        abort_with_error("lseek()", lseek(fd, SIZE_FILE_LISTCACHE, SEEK_SET) ==
                                    -1);                                                                                /* Makes a hole in the file */
        abort_with_error("write()", write(fd, &fd, 1) != 1);
    } else {
        fd = open_file(IMAGE_CACHE_FILE, O_RDWR);
    }

    char *map = mmap(NULL, SIZE_FILE_LISTCACHE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    abort_with_error("mmap()", map == MAP_FAILED);                                                                      /* Maps cache file */

    errno = 0;
    mlock(map,
          SIZE_FILE_LISTCACHE);                                                                                         /* Keeps file in memory in order to avoid I/O operations */
    printf("%s\n", strerror(errno));

    return map;

}

FILE *get_log() {

    FILE *fp;

    time_t timer;
    char time_buf[26];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);
    strftime(time_buf, 26, "%Y-%m-%d", tm_info);
    char *path = catenate_strings(SERVER_LOG_PATH, time_buf);

    fp = fopen(path, "a+");
    if (fp == NULL) {
        abort_with_error("fopen", fp == NULL);
    }

    return fp;
}

void read_config_file() {

    FILE *fp = open_fp(SERV_CONF, "r");
    int num, par;
    char *buf_line = memory_alloc(512), p;

    while(!feof(fp)) {
        if(fgets(buf_line, 512, fp) != NULL) {
            buf_line[511] = '\0';

            par = check_parameter(&buf_line);
            num = parse_int(buf_line);
            buf_line = memory_alloc(512);
            switch (par) {
                case CONF_NUMBER_THREAD:
                    num_thread_pool = num;
                    break;

                case CONF_MAX_CONN:
                    max_conn_db = num;
                    break;

                case CONF_PORT_SERV:
                    serv_port = num;
                    break;

                case CONF_BACKLOG:
                    backlog = num;
                    break;

                default:
                    break;
            }
        };
    }

    free(buf_line);
}

struct server_t *Server() {                                                                                             /* Allocates memory for the server */
    struct server_t *serverPtr = (struct server_t *) memory_alloc(sizeof(struct server_t));
    return serverPtr;
}

struct server_t *init_server() {

    struct server_t *serverPtr = Server();

    read_config_file();

    serverPtr->listen_sock = create_socket();
    set_socket_options(serverPtr->listen_sock, 0,
                       SO_REUSEADDR);
    set_server_address(serverPtr);
    bind_address(serverPtr->listen_sock, serverPtr->serv_addr);
    image_list();

    if (listen(serverPtr->listen_sock, backlog) < 0) {
        perror("Errore in listen");
        exit(EXIT_FAILURE);
    }

                                                                                     /* It prevents error on bind() for subsequent run of the program */
    log_fp = get_log();

    set_number_of_connections();                                                                                        /* Allows one connection per thread */

    pool = memory_alloc(4 * sizeof(struct pool_t));

    int i = 0;
    while(i < 4) {
        allocate_pool(
                num_thread_pool, pool + i);                                                                                           /* Preallocation of NUM_THREAD_POOL threads */
        i++;
    }

    //init_pollfd(
    //       serverPtr->pool->array_fd);                                                                                 /* Initialization of the pool of fds that are going to be checked in poll()*/
    //serverPtr->pool->array_fd[0].fd = serverPtr->listen_sock;
    //serverPtr->pool->array_fd[0].events = POLLIN;

    log_fp = get_log();

    file_map = get_cache_file();                                                                                        /* Creates the file that contains name of the images resized previously */
    seek_cache = 0;                                                                                                     /* Seek in the cache file */

    /******************** Icon ****************************/
    icon.fd = open_file(ICON_PATH, O_RDONLY);                                                                           /* The icon is opened once and for all because it is requested with high frequency */
    icon.image_name = ICON_PATH;
    icon.file_size = get_file_size(icon.fd);
    /*****************************************************/

    arrfd = memory_alloc(32 * sizeof(struct pollfd));
    init_pollfd(
            arrfd);                                                                                 /* Initialization of the pool of fds that are going to be checked in poll()*/
    arrfd[0].fd = serverPtr->listen_sock;
    arrfd[0].events = POLLIN;

    return serverPtr;
}