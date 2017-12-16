//
// Created by federico on 14/10/17.
//

#include <string.h>
#include <pthread.h>
#include <mysql.h>

#include "include/HandleDB.h"
#include "include/Utils.h"
#include "include/Strings.h"

static struct cache_query cache[256];
unsigned long long int idx_cache = 0;
pthread_mutex_t db_mtx[NUM_MTX_DB];
pthread_mutex_t db_mtx_cache;

void retrieve_dim_from_DB(struct request_t *request,
                          MYSQL *conn) {
    size_t len = 0;
    MYSQL_RES *res = NULL;
    MYSQL_ROW row = NULL;
    int i = 0;

    char tmp_query[128];
    if (strcmp(request->user_agent, "") != 0) {
        sprintf(tmp_query, "SELECT * FROM device WHERE useragent LIKE '%s%%';", request->user_agent);
    }

    /*************************** Search in cache ****************************************/
    get_mutex(&db_mtx_cache);
    while (i < 256) {                                                                                                   /* Search for result of the query in cache */

        if (cache[i].query != NULL) {

            if (strstr(cache[i].query, tmp_query) != NULL) {

                if (cache[i].state == NO_VALUE_IN_DB)
                    row = (MYSQL_ROW) -1;

                break;
            }
        }

        i++;
    }
    release_mutex(&db_mtx_cache);
    /***********************************************************************************/

    /* send SQL query */
    if (i == 256) {                                                                                                  /* If row wasn't assigned in the loop */

        /****************** Access DB *************************/
        mysql_ping(conn);

        if (mysql_query(conn, tmp_query)) {
            fprintf(stderr, "%s\n", mysql_error(conn));
        }
        /****************************************************/

        res = mysql_use_result(conn);

        if ((row = mysql_fetch_row(res)) != NULL) {                                                                     /* If data has been retrieved */

            request->colors = parse_int(row[7]);
            request->width = parse_int(row[3]);
            request->height = parse_int(row[2]);
            build_image_name_cache(&(request->cache_name),
                                   request->image_name,
                                   request->image_list->q,
                                   request->width,
                                   request->height,
                                   request->colors);                                                                    //Name used for caching
            //request->cache_name = strdup(image_name_cache);
            //request->cache_name = memory_alloc(strlen(image_name_cache) + 1);
            //sprintf(request->cache_name, "%s", image_name_cache);

            get_mutex(&db_mtx_cache);
                sprintf(cache[(idx_cache) % 256].query, "%s", tmp_query);                                                         /* Used simple FIFO queue */
                cache[(idx_cache) % 256].colors = request->colors;
                cache[(idx_cache) % 256].width = request->width;
                cache[(idx_cache) % 256].height = request->height;
                cache[(idx_cache) % 256].state = VALUE_DB;
                idx_cache++;
            release_mutex(&db_mtx_cache);

        } else {

            request->colors = 0;
            request->width = 0;
            request->height = 0;
            build_image_name_cache(&(request->cache_name),
                                   request->image_name,
                                   request->image_list->q,
                                   request->width,
                                   request->height,
                                   request->colors);    //Name used for caching
            //request->cache_name = strdup(image_name_cache);
            //request->cache_name = memory_alloc(strlen(image_name_cache) + 1);
            //sprintf(request->cache_name, "%s", image_name_cache);

            get_mutex(&db_mtx_cache);
                sprintf(cache[(idx_cache) % 256].query, "%s", tmp_query);

                cache[(idx_cache) % 256].colors = request->colors;
                cache[(idx_cache) % 256].width = request->width;
                cache[(idx_cache) % 256].height = request->height;
                //cache[(idx_cache) % 256].row = (MYSQL_ROW)-1;
                cache[(idx_cache) % 256].state = NO_VALUE_IN_DB;
                idx_cache++;
            release_mutex(&db_mtx_cache);
        }

        /* Release memory used to store results and close connection */
    } else if (row == (MYSQL_ROW) -1) {                                                                                 /* If row was assigned but there is no data in database */

        request->colors = 0;
        request->width = 0;
        request->height = 0;
        build_image_name_cache(&(request->cache_name),
                               request->image_name,
                               request->image_list->q,
                               request->width,
                               request->height,
                               request->colors);                                                                        /* Build name used to store and retrieve image from cache */

    } else {

        request->colors = cache[i].colors;
        request->width = cache[i].width;
        request->height = cache[i].height;
        build_image_name_cache(&(request->cache_name),
                               request->image_name,
                               request->image_list->q,
                               request->width,
                               request->height,
                               request->colors);                                                                        //Name used for caching

    }

    mysql_free_result(res);

}

void set_number_of_connections() {

    MYSQL *conn = connect_DB();

    char *query = memory_alloc(512);
    //char *str = convert_int_to_string(max_conn_db);

    //sprintf(query, "set global max_connections = %s;", str);
    sprintf(query, "set global max_connections = %d;", max_conn_db);

    query[strlen(query)] = '\0';

    if (mysql_query(conn, query)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
    }

    //free(str);
    bzero(query, 512);

    //str = convert_int_to_string(max_conn_db - 2);

    sprintf(query, "UPDATE mysql.user SET max_user_connections = %d WHERE user='root' AND host='localhost';", max_conn_db - 2);

    if (mysql_query(conn, query))
        fprintf(stderr, "%s\n", mysql_error(conn));

    mysql_close(conn);
    free(query);
    //free(str);

}

MYSQL *connect_DB() {

    char *server = "localhost";
    char *user = "root";
    char *password = "portento123";
    char *database = "devicedb";

    MYSQL *conn = mysql_init(NULL);

    /* Connect to database */
    if (!mysql_real_connect(conn, server, user, password, database, 3306, NULL, 0)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(EXIT_FAILURE);
    }

    return conn;
}

void init_mtx_db() {

    int i =0;

    while(i<NUM_MTX_DB) {

        init_mutex(db_mtx + i);
        i++;

    }

    init_mutex(&db_mtx_cache);

}