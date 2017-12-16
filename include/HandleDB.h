#ifndef IMAGEAPP_HANDLEDB_H
#define IMAGEAPP_HANDLEDB_H

#include <mysql.h>

#include "Request.h"

/**
 * In order to avoid accesses to database, a buffer
 * of struct cache_query's can be a solution
 */
struct cache_query {

    /*@{*/
    char query[256];                                                                                                    /**< Query that should be sent to dbms */
    int state;                                                                                                          /**< Says whether the query has been retrieved data or not */
    int colors;                                                                                                         /**< Number of colors retrieved from the database */
    int width;                                                                                                          /**< Width retrieved from the database */
    int height;                                                                                                         /**< Height retrieved from the database */
    float q;                                                                                                            /**< Quality of the image requested */
    /*@{*/

};


enum DB_stat {NO_VALUE_IN_DB, VALUE_DB};

/**
 * Function: retrieve_from_DB
 *
 * This function retrieves the dimensions of the display and saves them
 * in the request parameter
 *
 * @param request A request
 * @param conn Connection with the database
 * @return A connection to the database ready to use
 */
void retrieve_dim_from_DB(struct request_t *request,
                          MYSQL *conn);

/**
 * Function: connect_DB
 *
 * This function performs the connection to the database
 *
 * @param
 * @return A connection to the database ready to use
 */
MYSQL *connect_DB();

/**
 * Function: set_number_of_connections()
 *
 * This function updates the number of allowed parallel connections to database
 * in order to support the number of parallel queries made by threads
 *
 * @return void
 */
void set_number_of_connections();

/**
 * Function: init_mtx_
 *
 * This function initializes mutex used to access database
 *
 * @return void
 */
void init_mtx_db();

#endif //IMAGEAPP_HANDLEDB_H
