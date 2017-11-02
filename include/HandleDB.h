//
// Created by federico on 14/10/17.
//

#ifndef IMAGEAPP_HANDLEDB_H
#define IMAGEAPP_HANDLEDB_H

#include <mysql.h>

#include "Request.h"

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
void retrieve_dim_from_DB(struct request_t *request, MYSQL *conn);

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
 * @param
 * @return void
 */
void set_number_of_connections();
#endif //IMAGEAPP_HANDLEDB_H
