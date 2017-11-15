//
// Created by federico on 31/10/17.
//

#include <pthread.h>
#include <unistd.h>

#include "include/ThreadPool.h"
#include "include/Utils.h"
#include "include/HttpServer.h"
#include "include/HandleDB.h"

extern struct pollfd *arrfd;

void allocate_pool(int num_threads, struct pool_t *pool) {

    struct thread_data *pool_thread = (struct thread_data *) memory_alloc(num_threads * sizeof(struct thread_data));

    int i = 1;
    pthread_t tid = 0;

    /* Initializes every struct thread_data that are to be in the pool */
    while (i < num_threads) {

        (pool_thread[i]).message = (char **) memory_alloc(
                5 * sizeof(char *));                                                                                    /* Allocated five slots for messages */
        int msg = 0;
        while (msg < 5) {
            (pool_thread[i]).message[msg] = memory_alloc(HTTP_MESSAGE_SIZE);                                            /* Slots of 512 bytes */
            msg++;
        }
        (pool_thread[i]).tid = tid;

        printf("i db %d\n", i);

        (pool_thread[i]).connDB = connect_DB();                                                                         /* Every thread gets its connection to the db */
        (pool_thread[i]).used = -1;                                                                         /* Every thread gets its connection to the db */


        init_mutex(&((pool_thread[i]).mtx_msg_socket));
        init_mutex(&((pool_thread[i]).mtx_new_request));
        init_cond(&((pool_thread[i]).cond_msg));
        init_cond(&((pool_thread[i]).cond_timer));

        abort_with_error("pthread_create()",
                         pthread_create(&((pool_thread[i]).tid),
                                        NULL,
                                        handle_client,
                                        pool_thread + i) != 0);

        i++;
    }

    /* The poll is allocated and the pool of threads is assigned to the relative field*/
    pool->arr = pool_thread;
    pool->E = 1;
    pool->S = 1;

    /**************** Initialization of the mutex and condition variables ****************************************/
    if (pthread_mutex_init(&(pool->mtx), NULL) != 0) {
        fprintf(stderr, "Error initializing the mutex\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&(pool->cb_not_empty), NULL) != 0) {
        fprintf(stderr, "Error initializing condition variable\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&(pool->cb_not_full), NULL) != 0) {
        fprintf(stderr, "Error initializing condition variable\n");
        exit(EXIT_FAILURE);
    }
    /************************************************************************************************************/

}

int find_E_for_fd(struct pool_t *pool, int fd) {

    int i = 0;

    while (i < num_thread_pool) {

        if (arrfd[i].fd == fd)
            return i;
        i++;
    }

    if (i == num_thread_pool)
        return -1;

}

int get_free_slot_arrfd(struct pollfd *arrfd) {

    int i = 0;

    while (i < num_thread_pool) {

        if (arrfd[i].fd == -1)
            return i;
        i++;
    }

    if (i == num_thread_pool)
        return -1;

}

int get_E(struct pool_t *pool) {

    int i = 0;
    int nE;

    while (1) {
        while (i < num_thread_pool) {
            nE = (pool->E + i) % num_thread_pool;
            if (pool->arr[nE].used == -1) {
                pool->E = (nE + 1) % num_thread_pool;                                                                   /* Start from next index the second time */
                if(pool->E == 0)
                    pool->E += 1;
                //signal_cond(&(pool->cb_not_empty));
                return nE;
            }
            i++;
        }

        if (i == num_thread_pool) {
            get_mutex(&pool->mtx);
            wait_cond(&(pool->cb_not_full), &(pool->mtx));
            i = 0;
            release_mutex(&(pool->mtx));
        }
    }

}

int handle_timer(struct thread_data *td) {

    while (1) {

        if (td->timer == 0) {
            return 0;
        } else {

            get_mutex(&(td->mtx_new_request));
            if ((td->msg_received) == 1) {
                td->timer = td->timer - 1;
                release_mutex(&(td->mtx_new_request));
                printf("return timer %d\n", td->timer);
                return td->timer;
            }
            release_mutex(&(td->mtx_new_request));
            td->timer = td->timer - 1;
            sleep(1);
        }

    }

}

void get_mutex(pthread_mutex_t *mtx) {

    if (pthread_mutex_lock(mtx) != 0) {
        fprintf(stderr, "Error getting mutex\n");
        exit(EXIT_FAILURE);
    }

}

void release_mutex(pthread_mutex_t *mtx) {

    if (pthread_mutex_unlock(mtx) != 0) {
        fprintf(stderr, "Error releasing mutex\n");
        exit(EXIT_FAILURE);
    }

}

void wait_cond(pthread_cond_t *cond, pthread_mutex_t *mx) {
    if (pthread_cond_wait(cond, mx) != 0) {
        perror("cond_wait");
        exit(EXIT_FAILURE);
    }
}

void signal_cond(pthread_cond_t *cond) {

    if (pthread_cond_signal(cond) != 0) {
        perror("cond_wait");
        exit(EXIT_FAILURE);
    }

}

void init_mutex(pthread_mutex_t *mtx) {
    if (pthread_mutex_init(mtx, NULL) != 0) {
        fprintf(stderr, "Error in pthread_mutex_init()\n");
        exit(EXIT_FAILURE);
    }
}

void init_cond(pthread_cond_t *cond) {
    if (pthread_cond_init(cond, NULL) != 0) {
        fprintf(stderr, "Error in pthread_mutex_init()\n");
        exit(EXIT_FAILURE);
    }
}