/* From the example in the man page of pthread_setaffinity_np() function */
#define _GNU_SOURCE

#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <string.h>

#include "include/ThreadPool.h"
#include "include/Utils.h"
#include "include/HttpServer.h"
#include "include/HandleDB.h"
#include "include/Log.h"

void allocate_pool(int num_threads, struct pool_t **pool, int idx_pool) {

    struct thread_data *pool_thread = memory_alloc(num_threads * sizeof(struct thread_data));

    int i = 0;
    pthread_t tid = 0;

    /* Initializes every struct thread_data that are to be in the pool */
    while (i < num_threads) {

        (pool_thread[i]).message = memory_alloc(5 * sizeof(char *));                                                                                        /* Allocated five slots for messages */
        int msg = 0;
        while (msg < 5) {
            (pool_thread[i]).message[msg] = memory_alloc(HTTP_MESSAGE_SIZE);                                                                                 /* Slots of 512 bytes */
            msg++;
        }
        (pool_thread[i]).tid = tid;
        //(pool_thread[i]).idx_mtx_db = i % NUM_MTX_DB;

        printf("i db %d\n", i);

        (pool_thread[i]).connDB = connect_DB();                                                                         /* Every thread gets its connection to the db */
        (pool_thread[i]).used = -1;
        (pool_thread[i]).idx_pool = idx_pool;
        (pool_thread[i]).E = i;

        init_mutex(&((pool_thread[i]).mtx_new_request));
        init_cond(&((pool_thread[i]).cond_msg));

        i++;
    }

    /* The pool is allocated and the pool of threads is assigned to the relative field */
    struct pool_t *pool1 = memory_alloc(sizeof(struct pool_t));
    pool1->arr = pool_thread;
    pool1->E = 0;
    pool1->counter = 0;

    pool[idx_pool] = pool1;

    i = 0;
    while (i < num_threads) {

        abort_with_error("pthread_create()",
                         pthread_create(&((pool_thread[i]).tid),
                                        NULL,
                                        handle_client,
                                        pool_thread + i) != 0);

        i++;
    }

    /**************** Initialization of the mutex and condition variables ****************************************/
    if (pthread_mutex_init(&(pool1->mtx), NULL) != 0) {
        fprintf(stderr, "Error initializing the mutex\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&(pool1->cb_not_full), NULL) != 0) {
        fprintf(stderr, "Error initializing condition variable\n");
        exit(EXIT_FAILURE);
    }
    /************************************************************************************************************/

}

int set_thread_affinity(
        int core_id) {                                                                                                  /* This code has been retrieved from the man page of the pthread_setaffinity_np() function */

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_t current_thread = pthread_self();
    return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);

}

void close_client_with_status(struct thread_data *td, int idx_pool, int status) {

    td->used = -1;
    td->timer = 5;
    td->msg_received = 0;
    td->request = 1000;

    get_mutex(&mtx_max_d);
    max_descriptor--;
    release_mutex(&mtx_max_d);

    write_event_log(log_fp, status,
                    td->client_addr,
                    NULL);                                                                                  /* Keeps track of the client's connection in the log file */

    if(status == CONNECTION_CLOSED) {

        shutdown(td->conn_sd, SHUT_RDWR);
        if (close(td->conn_sd) == -1) {
            fprintf(stderr, "Error in close(), errno %d\n strerror %s\n", errno, strerror(errno));
        };
        //if(close(td->client_addr.sin_port) == -1) {
        //    fprintf(stderr, "Error in close(), errno %d\n strerror %s\n", errno, strerror(errno));
       // }

        get_mutex(&(pool[idx_pool]->mtx));
        signal_cond(&(pool[idx_pool]->cb_not_full));
        pool[idx_pool]->counter--;
        release_mutex(&(pool[idx_pool]->mtx));

    }

}

void *handle_pool(void *arg) {

    while(1) {

        get_mutex(&mtx_realloc);
            wait_cond(&cond_realloc, &mtx_realloc);

            int pp = 0;
            while (pp < size_pool) {

                allocate_pool(num_thread_pool, pool, (int)size_pool + pp);

                pthread_t tid;
                int *idx = memory_alloc(sizeof(int));
                init_mutex(&dispatcher[size_pool + pp].mtx_dispatch);
                init_cond(&dispatcher[size_pool + pp].cond_dispatch);
                dispatcher[size_pool + pp].idx_pool = size_pool + pp;
                (dispatcher + size_pool + pp)->idx_pool = pp;
                (dispatcher + size_pool + pp)->request = 0;
                *idx = size_pool + pp;
                abort_with_error("pthread_create", pthread_create(&tid, NULL, dispatch_job, idx) != 0);
                pp++;

            }

            size_pool += size_pool;
            signal_cond(&cond_realloc_end);
        release_mutex(&mtx_realloc);

    }

}

void *dispatch_job(void *arg) {

    int *i = (int *)arg;
    int idx_pool = *i;

    int nE, conn_sd;
    struct sockaddr_in client_addr;

    /**
     * These functions were commented because during the tests the program
     * crashes fewer times on average
     *
     * long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
     * set_thread_affinity(td->E % (int)num_cores);
     */

    while(1) {

        get_mutex(&(dispatcher + idx_pool)->mtx_dispatch);
        wait_cond(&((dispatcher + idx_pool)->cond_dispatch), &((dispatcher + idx_pool)->mtx_dispatch));
            conn_sd = ((dispatcher + idx_pool)->conn_sd);
            client_addr = ((dispatcher + idx_pool)->client_addr);
        release_mutex(&((dispatcher + idx_pool)->mtx_dispatch));


        /**************** Searching for a free slot *************************************/

        fprintf(stderr, "idx_pool %d\n", idx_pool);
        nE = get_E(
                pool, idx_pool);                                                                                        /* Finds a free slot in the ring buffer */

        /***** Save informations associated with thread ******/
        (((*(pool + idx_pool))->arr) + nE)->conn_sd = conn_sd;
        (((*(pool + idx_pool))->arr) + nE)->client_addr = client_addr;
        (((*(pool + idx_pool))->arr) + nE)->timer = 5;
        (((*(pool + idx_pool))->arr) + nE)->request = 1000;
        (((*(pool + idx_pool))->arr) + nE)->msg_received = 1;
        (((*(pool + idx_pool))->arr) + nE)->used = 1;
        (*(pool + idx_pool))->counter++;

        /******************************************************************************/

        get_mutex(&((((*(pool + idx_pool))->arr) + nE)->mtx_new_request));
            signal_cond(&((((*(pool + idx_pool))->arr) + nE)->cond_msg));
        release_mutex(&((((*(pool + idx_pool))->arr) + nE)->mtx_new_request));

    }

}

int get_E(struct pool_t **pool, int idx_pool) {

    int i = 0;
    int nE;

    while(1) {

        while (i < num_thread_pool) {

            nE = (pool[idx_pool]->E + i) % num_thread_pool;
            if (pool[idx_pool]->arr[nE].used == -1) {
                pool[idx_pool]->E = (nE + 1) %
                                    num_thread_pool;                                                                    /* Start from next index the second time */
                pool[idx_pool]->counter++;
                return nE;
            }

            i++;

        }

        fprintf(stderr, "in idxpool %d i == numthrea i=%d\n", idx_pool, i);
        get_mutex(&(pool[idx_pool]->mtx));
            wait_cond(&(pool[idx_pool]->cb_not_full), &(pool[idx_pool]->mtx));
            i = 0;
        release_mutex(&(pool[idx_pool]->mtx));

    }

}

int handle_timer(int idx_pool, int E, struct pollfd *pfd) {

    int ready;

    while (1) {

        ready = poll(pfd, 1, 1000);

        get_mutex(&mtx_realloc);
        if ((((*(pool + idx_pool))->arr) + E)->timer == 0) {

            if (ready == 1) {
                
                ((*(pool + idx_pool))->arr + E)->msg_received = 1;
                ((*(pool + idx_pool))->arr + E)->request = ((*(pool + idx_pool))->arr + E)->request - 1;
                ((*(pool + idx_pool))->arr + E)->timer = 5;
                release_mutex(&mtx_realloc);
                return 1;

            } else {

                release_mutex(&mtx_realloc);
                return 0;

            }

        } else {

            if (ready == 1) {

                ((*(pool + idx_pool))->arr + E)->timer = ((*(pool + idx_pool))->arr + E)->timer - 1;
                ((*(pool + idx_pool))->arr + E)->msg_received = 1;
                ((*(pool + idx_pool))->arr + E)->request = ((*(pool + idx_pool))->arr + E)->request - 1;
                release_mutex(&mtx_realloc);
                return ((*(pool + idx_pool))->arr + E)->timer;
            }

            ((*(pool + idx_pool))->arr + E)->timer = ((*(pool + idx_pool))->arr + E)->timer - 1;
            release_mutex(&mtx_realloc);
            sleep(1);

        }
    }

}

void wait_for_connection(struct thread_data *td) {

    get_mutex(&td->mtx_new_request);
    if (td->msg_received != 1) {
        wait_cond(&td->cond_msg,
                  &td->mtx_new_request);                                                                                /* Wait until a message has been received */
    }
    release_mutex(&td->mtx_new_request);

}