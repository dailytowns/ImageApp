//
// Created by federico on 03/10/17.
//
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/ThreadPool.h"
#include "include/Utils.h"
#include "include/HandleDB.h"

struct thread_pool *allocate_pool(int num_threads) {

    struct thread_data *pool_thread = (struct thread_data *) memory_alloc(num_threads * sizeof(struct thread_data));

    int i = 0;
    pthread_t tid = 0;

    /* Initializes every struct thread_data that are to be in the pool */
    while (i < num_threads) {

        (pool_thread[i]).message = (char **) memory_alloc(5 * sizeof(char *));                                               //Allocated five slots for messages
        (pool_thread[i]).fd = (int *) memory_alloc(5 * sizeof(int));                                               //Allocated five slots for messages
        (pool_thread[i]).tid = tid;
        (pool_thread[i]).conn_sd = 0;
        (pool_thread[i]).idx = 0;

        printf("i db %d\n", i);

        (pool_thread[i]).connDB = connect_DB();

        init_mutex(&((pool_thread[i]).mtx_msg_socket));
        init_mutex(&((pool_thread[i]).mtx_new_request));
        init_cond(&((pool_thread[i]).cond_no_msg));

        i++;
    }

    /* The poll is allocated and the pool of threads is assigned to the relative field*/
    struct thread_pool *pool = memory_alloc(sizeof(struct thread_pool));
    pool->td_pool = pool_thread;
    pool->E = 0;
    pool->S = 0;

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

    pool->get_E = get_E;


    return pool;
}

int get_E(struct thread_pool *pool, int S, int E) {

    int i = 0;
    int nE = (E+1) % NUM_THREAD_POOL;

    while(1) {

        if(pool->slot_used[nE] == 0)                                                                                    /* Try to get index fast */
            return nE;

        while (i < NUM_THREAD_POOL) {
            if (pool->slot_used[(S + i)%NUM_THREAD_POOL] == 0) {
                signal_cond(&(pool->cb_not_empty));
                return i;
            }
            i++;
        }

        if (i == NUM_THREAD_POOL) {
            wait_cond(&(pool->cb_not_full), &(pool->mtx));
            i = 0;
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