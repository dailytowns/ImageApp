#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/Config.h"
#include "include/Utils.h"

int check_parameter(char *buf, int *counter) {

    int value_parsed = -1;

    /* Choose the correct parameter to be set */
    if(strstr(buf, "NUM_THREAD_POOL_START"))
        value_parsed = CONF_NUMBER_THREAD;
    else if(strstr(buf, "MAX_CONNECTION_DB"))
        value_parsed = CONF_MAX_CONN;
    else if(strstr(buf, "SIZE_POOL"))
        value_parsed = CONF_SIZE_POOL;
    else if(strstr(buf, "SERV_PORT"))
            value_parsed = CONF_PORT_SERV;
    else if(strstr(buf, "BACKLOG"))
        value_parsed = CONF_BACKLOG;

    /* Compute the correct index to start parsing the value */
    if(value_parsed != -1) {
        while (*buf != ' ' && *buf != '\t' && *buf != '\n') {
            (*counter)++;
            buf++;
        }
        return value_parsed;
    }

    return -1;
}

void parse_config_file() {

    FILE *fp = open_fp(SERV_CONF, "r");
    int num, par;
    char buf_line[512];
    int counter;

    while (!feof(fp)) {

        counter = 0;                                                                                                    /* Index where the parsing starts */

        if (fgets(buf_line, 512, fp) != NULL) {
            buf_line[511] = '\0';

            par = check_parameter(buf_line, &counter);                                                                  /* Find which parameter is going to be set */
            num = parse_int(buf_line + counter);                                                                        /* Parse the value written in the file */
            bzero(buf_line, 512);                                                                                       /* Clean buffer */

            switch (par) {                                                                                              /* Assign the correct parameter */

                case CONF_NUMBER_THREAD:
                    num_thread_pool = num;
                    break;

                case CONF_MAX_CONN:
                    max_conn_db = num;
                    break;

                case CONF_PORT_SERV:
                    serv_port = num;
                    break;

                case CONF_BACKLOG:
                    backlog = num;
                    break;

                case CONF_SIZE_POOL:
                    size_pool = (size_t) num;
                    break;

                case CONF_SIZE_ARRFD:
                    size_arrfd = num;
                    break;

                default:
                    break;
            }
        };
    }
}