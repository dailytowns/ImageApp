//
// Created by federico on 03/10/17.
//

#ifndef IMAGEAPP_THREADPOOL_H
#define IMAGEAPP_THREADPOOL_H

#include <mysql.h>
#include <netinet/in.h>

#include "Config.h"

struct thread_data {
    int *fd;
    int conn_sd;
    char **message;                                     //Deals with (?) pipeline
    pthread_t tid;
    pthread_mutex_t mtx_msg_socket;
    pthread_mutex_t mtx_new_request;
    pthread_cond_t cond_no_msg;
    int idx;
    int msg_received;
    int E;
    pthread_cond_t cond_msg;
    MYSQL *connDB;
    struct sockaddr_in client_addr;
};

/**
 * The pool of threads that is going to be used in the program.
 * In this case, it is implemented as a ring buffer
 */
struct thread_pool {
    /*@{*/
    int E;                                                                                                              /**< The end index of the ring buffer */
    int S;                                                                                                              /**< The start index of the ring buffer */
    short slot_used[NUM_THREAD_POOL];                                                                                   /**< Array that keeps track of the slots used in the ring buffer */
    pthread_mutex_t mtx;                                                                                                /**< Mutex useful for mutual exclusive access to the buffer */
    pthread_cond_t cb_not_full;                                                                                         /**< Condition variable that is used to signal or wait for buffer not full */
    pthread_cond_t cb_not_empty;                                                                                        /**< Condition variable that is used to signal or wait for buffer not empty */
    struct thread_data *td_pool;                                                                                        /**< Pointer to the ring buffer */

    int (*get_E)(struct thread_pool *pool, int S, int E);                                                               /**< Function pointer */
    /*@{*/
};

/**
 * Function: get_E
 *
 * It finds a free slot in the ring buffer and returns its index
 *
 * @param pool The pool of threads
 * @param S The start index
 * @param E The end index
 * @return The index of the free slot
 */
int get_E(struct thread_pool *pool, int S, int E);

/**
 * Function: allocate_pool
 *
 * It allocates the pool of threads used when a client connects to the server
 *
 * @param num_threads Number of threads preallocated
 * @return A pointer to the array of threads preallocated
 */
struct thread_pool *allocate_pool(int num_threads);

/**
 * Function: init_mutex
 *
 * It is a wrapper for the syscall that initializes the mutex variable
 *
 * @param mtx Pointer to the mutex that is going to be initialized
 * @return void
 */
void init_mutex(pthread_mutex_t *mtx);

/**
 * Function: init_cond
 *
 * It is a wrapper for the syscall that initializes the condition variable
 *
 * @param cond Pointer to the condition variable that is going to be initialized
 * @return void
 */
void init_cond(pthread_cond_t *cond);

/**
 * Function: wait_cond
 *
 * It is a wrapper for the syscall that waits for the condition variable
 *
 * @param cond Pointer to the condition variable that is going to be initialized
 * @param mtx Pointer to the mutex associated to the condition variable
 * @return void
 */
void wait_cond(pthread_cond_t *cond, pthread_mutex_t *mtx);

/**
 * Function: signal_cond
 *
 * It is a wrapper for the syscall that releases the condition variable
 *
 * @param cond Pointer to the condition variable that is going to be signaled
 * @return void
 */
void signal_cond(pthread_cond_t *cond);

/**
 * Function: get_mutex
 *
 * It is a wrapper for the syscall that gets the pthread mutex
 *
 * @param mtx Pointer to the mutex
 * @return void
 */
void get_mutex(pthread_mutex_t *mtx);

/**
 * Function: release_mutex
 *
 * It is a wrapper for the syscall that releases the pthread mutex
 *
 * @param mtx Pointer to the mutex
 * @return void
 */
void release_mutex(pthread_mutex_t *mtx);

/**
 * Function: find_E_for_fd
 *
 * Search in the pool of threads for a connection opened before
 * and returns its index
 *
 * @param pool pool of threads
 * @param S start index
 * @param E end index
 * @param fd file descriptor to be found
 *
 * @return index of slot corresponding to the file descriptor
 */
int find_E_for_fd(struct thread_pool *pool, int S, int E, int fd);
#endif //IMAGEAPP_THREADPOOL_H
