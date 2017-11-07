//
// Created by federico on 04/11/17.
//

#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "include/Config.h"

int check_parameter(char **buf) {

    size_t size_str = strlen(*buf);
    int i = 0, j = 0;

    while(i < size_str) {

        char c = *(*buf + i);

        if(isalpha(c) != 0) {
            if(**buf == 'N') {
                while(**buf != ' ' && **buf != '\t' && **buf != '\n')
                    *buf = *buf + 1;
                return CONF_NUMBER_THREAD;
            } else if(**buf == 'M') {
                while(**buf != ' ' && **buf != '\t' && **buf != '\n')
                    *buf = *buf + 1;
                return CONF_MAX_CONN;
            } else if(**buf == 'S' ) {
                while(**buf != ' ' && **buf != '\t' && **buf != '\n')
                    *buf = *buf + 1;
                return CONF_PORT_SERV;
            } else if(**buf == 'B') {
                while(**buf != ' ' && **buf != '\t' && **buf != '\n')
                    *buf = *buf + 1;
                return CONF_BACKLOG;
            } else if(**buf == '#') {
                break;
            }
        }

        i++;
    }

    return -1;
}