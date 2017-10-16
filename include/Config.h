//
// Created by federico on 02/10/17.
//

#ifndef IMAGEAPP_CONFIG_H
#define IMAGEAPP_CONFIG_H

#define HTTP_MESSAGE_SIZE 512

#define NUM_THREAD_POOL 256                                                                                             /* In MySQL the max_connection-th connection is for root user */

#define MAX_CONNECTION_DB 258                                                                                           /* 256 + root connection + eventual connection to MySQL workbench*/

#define SERV_PORT	5193
#define BACKLOG		10
#define MAXLINE		1024

#define SERV_CONF "/home/federico/CLionProjects/ImageApp/serv.conf"
#define SERVER_LOG_PATH "/home/federico/CLionProjects/ImageApp/log/logfile"
#define IMAGE_DIR "/home/federico/CLionProjects/ImageApp/images/"
#define IMAGE_CACHE "/home/federico/CLionProjects/ImageApp/cache/"
#define IMAGE_LIST "/home/federico/CLionProjects/ImageApp/res/imagelist"
#define IMAGE_CACHE_FILE "/home/federico/CLionProjects/ImageApp/res/list_cache"
#define ICON_PATH "/home/federico/CLionProjects/ImageApp/images/favicon.ico"

#define ICON_NAME "favicon.ico"

#define IMAGE_NAME_PREALLOCATION 64
#define USERAGENT_PREALLOCATION 128

#define SIZE_FILE_LISTCACHE 65536                   //Maximum amount of memory lockable

#endif //IMAGEAPP_CONFIG_H
