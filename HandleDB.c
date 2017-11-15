//
// Created by federico on 14/10/17.
//

#include <string.h>
#include <pthread.h>
#include <mysql.h>

#include "include/HandleDB.h"
#include "include/Utils.h"
#include "include/Strings.h"

struct cache_query {
    char *query;
    MYSQL_ROW row;
    int state;
};

static struct cache_query cache[256];
unsigned long long int idx_cache = 0;
pthread_mutex_t db_mtx;

void retrieve_dim_from_DB(struct request_t *request, MYSQL *conn) {

    MYSQL_RES *res = NULL;
    MYSQL_ROW row = NULL;
    int i = -1;

    char *image_name_cache = NULL;
    image_name_cache = (char *) memory_alloc(256 * sizeof(char));

    char *tmp_query = (char *)memory_alloc(256 * sizeof(char));
    if (strcmp(request->user_agent, "") != 0)
        sprintf(tmp_query, "SELECT * FROM device WHERE useragent LIKE '%s%%';", (request->user_agent));

    while (++i < 256) {                                                                                                 /* Search for result of the query in cache */
        if (cache[i].query != NULL) {
            if (strcmp(tmp_query, cache[i].query) == 0) {

                if (cache[i].row != NULL && cache[i].state == VALUE_DB) {
                    row = cache[i].row;
                } else row = (MYSQL_ROW) -1;

                break;
            }
        }
    }

    /* send SQL query */
    if (row == NULL) {                                                                                                  /* If row wasn't assigned in the loop */

        /****************** Access DB *************************/
        get_mutex(&db_mtx);
        mysql_ping(conn);

        if (mysql_query(conn, tmp_query)) {
            fprintf(stderr, "%s\n", mysql_error(conn));
        }
        release_mutex(&db_mtx);
        /****************************************************/

        res = mysql_use_result(conn);

        if ((row = mysql_fetch_row(res)) != NULL) {                                                                     /* If data has been retrieved */
            request->colors = parse_int(row[7]);
            request->width = parse_int(row[3]);
            request->height = parse_int(row[2]);
            build_image_name_cache(&image_name_cache,
                                   request->image_name,
                                   request->image_list->q,
                                   request->width,
                                   request->height,
                                   request->colors);                                                                    //Name used for caching
            request->cache_name = strdup(image_name_cache);

            cache[(idx_cache++) % 256].query = strdup(tmp_query);                                                       /* Used simple FIFO queue */
            cache[(idx_cache++) % 256].row = row;
            cache[(idx_cache++) % 256].state = VALUE_DB;

        } else {
            request->colors = 0;
            request->width = 0;
            request->height = 0;
            build_image_name_cache(&image_name_cache,
                                   request->image_name,
                                   request->image_list->q,
                                   request->width,
                                   request->height,
                                   request->colors);    //Name used for caching
            request->cache_name = strdup(image_name_cache);
            cache[(idx_cache++) % 256].query = strdup(tmp_query);
            cache[(idx_cache++) % 256].state = NO_VALUE_IN_DB;
        }

        /* Release memory used to store results and close connection */
    } else if (row == (MYSQL_ROW) -1) {                                                                                 /* If row was assigned but there is no data in database */
        request->colors = 0;
        request->width = 0;
        request->height = 0;
        build_image_name_cache(&image_name_cache,
                               request->image_name,
                               request->image_list->q,
                               request->width,
                               request->height,
                               request->colors);                                                                        /* Build name used to store and retrieve image from cache */
        request->cache_name = strdup(image_name_cache);
    }

    mysql_free_result(res);
    free(tmp_query);
}

void set_number_of_connections() {

    MYSQL *conn = connect_DB();

    char *query = memory_alloc(512);

    sprintf(query, "set global max_connections = %s;", convert_int_to_string(258));

    query[strlen(query)] = '\0';

    if (mysql_query(conn, query)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        //   return(0);
    }

    sprintf(query, "UPDATE mysql.user SET max_user_connections = %d WHERE user='root' AND host='localhost';", 256);

    query[strlen(query)] = '\0';

    if (mysql_query(conn, query)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        //   return(0);
    }

    mysql_close(conn);

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
