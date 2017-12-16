#ifndef IMAGEAPP_UTILS_H
#define IMAGEAPP_UTILS_H

#include <semaphore.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>

/**
 * Enumeration that helps identifying if an image has or not been transformed
 * before
 */
enum cache_file {CACHED_IMAGE, NOT_CACHED_IMAGE};

/**
 * Function: abort_with_error
 *
 * It aborts the program if the values are equal and prints in the stderr
 * an error message
 *
 * @param Function that could fail
 * @param cond If verified, the program is aborted
 * @return A pointer that points the memory allocated
 */
void abort_with_error(char *caller, int cond);

/**
 * Function: memory_alloc
 *
 * It deals with the memory allocation and the error detection in
 * this operation
 *
 * @param size Number of bytes to be allocated
 * @return A pointer that points the memory allocated
 */
void *memory_alloc(size_t size);

/**
 * Function: image_list
 *
 * It deals with the creation of the list of files present in the image
 * directory
 *
 * @param
 * @return void
 */
void image_list();

/**
 * Function: open_fp
 *
 * It opens a file and checks for error in this operation
 *
 * @param path Path of the file to be opened
 * @param mode It specifies the modality of using the file.
 * @return A file pointer pointing to the file
 */
FILE *open_fp(const char *path, const char *mode);

/**
 * Function: open_file
 *
 * It opens a file and checks for error in this operation
 *
 * @param path Path of the file to be opened
 * @param mode It specifies the modality of using the file.
 * @return A file descriptor of the file
 */
int open_file(const char *path, int mode);

/**
 * Function: parse_int
 *
 * It parses a string to an integer
 *
 * @param s String to be parsed
 * @return Integer corresponding to s
 */
int parse_int(char *s);

/**
 * Function: get_file_size
 *
 * Computes the size of the file
 *
 * @param fd file descriptor
 * @return file size
 */
off_t get_file_size(int fd);

/**
 * Function: find_file_in_cache
 *
 * Scan cache to check if image requested has been
 * requested before
 *
 * @param cache_path Path in cache
 * @param map mapped file
 * @param idx Index to retrieve file descriptor
 * @return status code
 */
int find_file_in_cache(char *cache_path, char *map, int *idx);

/**
 * Function: read_block
 *
 * This function could be used in pair with write_block() to read bytes
 * from the file to be sent. In the function send_image() it is replaced by
 * sendfile()
 *
 * @param fd File descriptor of the image to be sent
 * @param buf Buffer that will contain bytes to be read
 * @param size Number of bytes to be read
 * @return Status of reading
 */
ssize_t read_block(int fd, char *buf, unsigned long size);

/**
 * Function: write_block
 *
 * This function could be used in pair with read_block() to send bytes
 * through a socket. In the function send_image() it is replaced by
 * sendfile()
 *
 * @param sock_fd Socket in which the bytes are sent
 * @param buf Buffer containing bytes to be sent
 * @param size Number of bytes to be sent
 * @return Status of sending
 */
int write_block(int sock_fd, char *buf, unsigned long size);

/**
 * Function: create_socket
 *
 * It creates a socket
 *
 * @param
 * @return Socket's file descriptor
 */
int create_socket();

/**
 * Function: set_socket_options
 *
 * It sets the options of the socket that are used in the program
 *
 * @param sock_fd The socket that will be set
 * @param keep_alive If it is passed as a parameter, the relative option will be set
 * @param reuse_addr If it is passed as a parameter, the relative option will be set
 * @return void
 */
void set_socket_options(int sockfd, int keep_alive, int reuse_addr);

/**
 * Function: set_server_address
 *
 * It sets the address family of the server, its ip address and the
 * port that is used to listen for connections
 *
 * @param server It is the struct that abstracts a server
 * @return void
 */
//void set_server_address(struct server_t *server);

/**
 * Function: bind_address
 *
 * It binds the listening socket of the server with the ip address of
 * the server in order to demultiplex the packets correctly
 *
 * @param listenSock The socket used to listen for connections
 * @param servaddr The ip address of the server
 * @return void
 */
void bind_address(int listenSock, struct sockaddr_in serv_addr);

/**
 * Function: get_cache_file
 *
 * It checks if the cache file already exists. If not, it creates it
 * and in both cases maps the file in memory
 *
 * @return A pointer to the file mapped
 */
char *get_cache_file();

/**
 * Function: get_log
 *
 * It creates a log file, exclusively, named with the today's date
 *
 * @return A pointer to the log file
 */
FILE *get_log();

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
#endif //IMAGEAPP_UTILS_H
