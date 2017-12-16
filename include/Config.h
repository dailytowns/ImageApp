#ifndef IMAGEAPP_CONFIG_H
#define IMAGEAPP_CONFIG_H

#define HTTP_MESSAGE_SIZE 512                                                                                           /* Bytes allocated per message */

int num_thread_pool;
int max_conn_db;
int backlog;
int serv_port;
size_t size_pool;
int size_arrfd;

/********************************** Parameters for database access ****************************************************/
#define NUM_MTX_DB 16
#define NUM_THREAD_POOL  64                                                                                             /* In MySQL the max_connection-th connection is for root user */
#define MAX_CONNECTION_DB 66                                                                                            /* 64 + root connection + eventual connection to MySQL workbench*/
#define MAX_FD 128

/********************************* Principal paths ********************************************************************/

#define SERV_CONF "../res/serv.conf"
#define SERVER_LOG_PATH "../log/"
#define IMAGE_DIR "../images/"
#define IMAGE_CACHE "../cache/"
#define IMAGE_LIST "../res/imagelist"
#define IMAGE_CACHE_FILE "../res/list_cache"
#define ICON_PATH "../images/favicon.ico"

//#define SERV_CONF "./res/serv.conf"
//#define SERVER_LOG_PATH "./log/"
//#define IMAGE_DIR "./images/"
//#define IMAGE_CACHE "./cache/"
//#define IMAGE_LIST "./res/imagelist"
//#define IMAGE_CACHE_FILE "./res/list_cache"
//#define ICON_PATH "./images/favicon.ico"
#define ICON_NAME "favicon.ico"

/**********************************************************************************************************************/

#define IMAGE_NAME_PREALLOCATION 64
#define USERAGENT_PREALLOCATION 256
#define SIZE_FILE_LISTCACHE 16384                                                                                       /* Maximum amount of memory lockable */

enum parameters_conf {CONF_NUMBER_THREAD,
    CONF_MAX_CONN,
    CONF_PORT_SERV,
    CONF_BACKLOG,
    CONF_SIZE_POOL,
    CONF_SIZE_ARRFD};

/**
 * Function: check_parameter
 *
 * This function performs the parsing of the configuration file
 *
 * @param buf_line A prealloced array of char
 * @param counter Number of letters before the value
 * @return An integer identifing the parameter parsed
 */
int check_parameter(char *buf_line, int *counter);

void parse_config_file();
#endif //IMAGEAPP_CONFIG_H
