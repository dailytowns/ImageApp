#ifndef IMAGEAPP_CONFIG_H
#define IMAGEAPP_CONFIG_H

#define HTTP_MESSAGE_SIZE 512                                                                                           /* Bytes allocated per message */
int num_thread_pool;
int max_conn_db;
int backlog;
int serv_port;
/********************************** Parameters for database access ****************************************************/

#define NUM_THREAD_POOL  64                                                                                             /* In MySQL the max_connection-th connection is for root user */
#define MAX_CONNECTION_DB 66                                                                                            /* 64 + root connection + eventual connection to MySQL workbench*/

/********************************** Socket parameters ******************************************************************/

#define SERV_PORT	5193                                                                                                /* Listen port */
#define BACKLOG		20                                                                                                  /* Maximum number of connection that can wait to be accept()ed*/

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
#define USERAGENT_PREALLOCATION 128
#define SIZE_FILE_LISTCACHE 65536                                                                                       /* Maximum amount of memory lockable */

enum parameters_conf {CONF_NUMBER_THREAD, CONF_MAX_CONN, CONF_PORT_SERV, CONF_BACKLOG};

int check_parameter(char **buf_line);

#endif //IMAGEAPP_CONFIG_H
