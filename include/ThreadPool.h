//
// Created by federico on 31/10/17.
//
#ifndef IMAGEAPP_THREADPOOL_H
#define IMAGEAPP_THREADPOOL_H

#include <mysql.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>

#include "Config.h"
#include "HttpServer.h"

FILE *log_fp;
nfds_t max_descriptor;
pthread_mutex_t mtx_max_d;

/**
 * This struct abstracts a slot in the circular buffer used to handle the clients
 * requesting images to server
 */
struct thread_data {

    /*@{*/
    int conn_sd;                                                                                                        /**< Connection handled by the thread */
    char **message;                                                                                                     /**< Array of strings to handle multiple requests at the same time */
    pthread_t tid;                                                                                                      /**< Thread identifier */
    pthread_mutex_t mtx_new_request;                                                                                    /**< Mutex used to get the condition variable for a new request */
    pthread_cond_t cond_msg;                                                                                            /**< Condition variable on which thread waits for a request */
    int idx;                                                                                                            /**< Index of the slot of the message */
    int msg_received;                                                                                                   /**< Flag checked to choose if thread can stop execution or not */
    int E;                                                                                                              /**< Index in the pool of the thread */
    MYSQL *connDB;                                                                                                      /**< Connection to database */
    struct sockaddr_in client_addr;                                                                                     /**< Address of the client */
    int timer;                                                                                                          /**< Timer for the keep-alive function */
    int request;                                                                                                        /**< Number of requests that client can make on the same connection */
    int idx_pool;                                                                                                       /**< Index of the pool in which the thread is stored */
    volatile int used;                                                                                                  /**< Flag checked to assign a thread to a client */
    /*@{*/

};

/**
 * The pool of threads that is going to be used in the program.
 * In this case, it is implemented as a ring buffer
 */
struct pool_t {

    /*@{*/
    int E;                                                                                                              /**< The end index of the ring buffer */
    pthread_mutex_t mtx;                                                                                                /**< Mutex useful for mutual exclusive access to the buffer */
    pthread_cond_t cb_not_full;                                                                                         /**< Condition variable that is used to signal or wait for buffer not full */
    struct thread_data *arr;                                                                                            /**< Pointer to the ring buffer */
    int counter;                                                                                                        /**< Number of slots occupied */
    /*@{*/

};

/**
 * Layer of communication between the main loop and the dispatcher
 * functionality
 */
struct dispatcher {

    /*@{*/
    pthread_mutex_t mtx_dispatch;                                                                                       /**< Mutex useful for the condition variable to which it is attached */
    pthread_cond_t cond_dispatch;                                                                                       /**< Condition variable that if signaled triggers the function of assigning a client to a thread */
    int idx_pool;                                                                                                       /**< Index of the pool chosen in the main loop */
    int conn_sd;                                                                                                        /**< Socket assigned to handle the client */
    int request;                                                                                                        /**< Number of requests available for the client */
    struct sockaddr_in client_addr;                                                                                     /**< Address of the client */
    /*@{*/

};

/**
 * Function: set_thread_affinity
 *
 * This functions is a wrapper for the system call pthread_setaffinity_np() that performs
 * the binding of a thread to a processor
 *
 * @param core_id Integer identifying the processor to be bound to a thread
 * @return Error status
 */
int set_thread_affinity(int core_id);

/**
 * Function: handle_pool
 *
 * This function deals with the handling of the number of pools that
 * are in memory
 *
 */
void *handle_pool(void *arg);

/**
 * Function: handle_timer
 *
 * This function deals with receiving a second request from the client during the
 * keep-alive timer
 *
 * @param idx_pool Index of the pool in which the thread is stored
 * @param E Index of the thread in the pool
 * @param pfd Struct to be poll()ed in the function
 * @return Status of the timer
 */
int handle_timer(int idx_pool, int E, struct pollfd *pfd);

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
int get_E(struct pool_t **pool, int idx_pool);

/**
 * Function: allocate_pool
 *
 * It allocates the pool of threads used when a client connects to the server
 *
 * @param num_threads Number of threads preallocated
 * @return A pointer to the array of threads preallocated
 */
void allocate_pool(int num_threads, struct pool_t **pool, int idx_pool);

/**
 * Function: dispatch_job
 *
 * This function choose a free slot in the pool that was assigned to the dispatcher
 * and unlock the pertinent thread
 *
 */
void *dispatch_job(void *arg);

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
int find_E_for_fd(struct pool_t **pool, int idx_pool, int fd);

/**
 * Function: close_client_with_status
 *
 * Wrapper for the close() function with resetting fields of
 * the struct thread_data passed
 *
 * @param td struct associated with the client
 * @param idx_pool index of the pool occupied by the struct
 * @param status status of the connection, in this case is only set to CONNECTION_CLOSED
 */
void close_client_with_status(struct thread_data *td, int idx_pool, int status);

/**
 * Function: wait_for_connection
 *
 * The struct associated with the client contains the condition variable
 * on which it is waited the connection. It returns only when signaled
 * by the dispatcher thread
 *
 * @param td struct associated with the client
 */
void wait_for_connection(struct thread_data *td);
#endif //IMAGEAPP_THREADPOOL_H
