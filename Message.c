#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/sendfile.h>

#include "include/Message.h"
#include "include/Config.h"
#include "include/Utils.h"
#include "include/HandleImage.h"
#include "include/Strings.h"

struct Request *create_request() {

    struct Request *request = memory_alloc(sizeof(struct Request));
    request->user_agent = memory_alloc(USERAGENT_PREALLOCATION * sizeof(char));
    request->image_name = memory_alloc(IMAGE_NAME_PREALLOCATION * sizeof(char));
    request->image_list = create_mime_list(4);

    return request;

}

int receive_message(struct thread_data *td, int idx) {

    ssize_t v = 0, received = 0;
    size_t tot = HTTP_MESSAGE_SIZE;
    int error;
    int retry = 2;                         //avoid errno = EAGAIN

    struct timespec start, finish;
    double elapsed;

    clock_gettime(CLOCK_MONOTONIC, &start);
    while (retry >= 0) {

        if (retry == 0 && v == -1)
            return EMPTY_MESSAGE;

        errno = 0;
        v = recv(td->conn_sd, td->message[idx] + received, tot - received, 0);
        error = errno;

        if (v == -1 || error != 0) {
            if ((error == EINTR) || (error == EAGAIN)) {
                retry--;
                continue;
            } else {
                fprintf(stderr, "recv() in receive_message(), errno = %d", error);
                exit(EXIT_FAILURE);
            }
        }

        /*In HTTP/1.1 there is not a content length header, so it is performed a raw control
         *to check if the message is completely arrived*/
        if (strcmp(td->message[idx], "") != 0) {
            char *tmp1 = (strstr(td->message[idx], "User-Agent"));
            char *tmp2 = (strstr(td->message[idx], "Accept: "));
            char *tmp3 = (strstr(td->message[idx], "Connection: "));
            if (tmp1 && tmp2 && tmp3) {
                ((td->message)[idx])[HTTP_MESSAGE_SIZE - 1] = '\0';

                return REQUEST_RECEIVED;
            }
        }


        received += v;

        if (v <= 0) {

            if (v == 0) {
                // connection closed
                printf("selectserver: socket %d hung up\n", td->conn_sd);

                shutdown(td->conn_sd, SHUT_RDWR);
                close(td->conn_sd);                                                                                     /* Both cause of error or closed connection, bye! */

                return EMPTY_MESSAGE;

            } else {
                perror("recv");
            }

        }
    }

}

int get_command_line(const char *message, char **firstline) {

    int i = 0;
    size_t size = 128;

    char *tmp_firstline = NULL;
    tmp_firstline = (char *) memory_alloc(size * sizeof(char));

    while (1) {
        if (message[i] != '\r') {
            tmp_firstline[i] = message[i];
        } else {
            tmp_firstline[i] = '\0';                                                                                    /* '\0' is the end of string. This assignment deals with SIGSEGV strcpy()s or in strlen()*/
            break;
        }
        i++;

        if(i == size) {                                                                                                 /* Reallocation in case of a name too long */
            if(size == (size + size))
                return REQUEST_TOO_LONG;
            size += size;
            char *p = realloc(tmp_firstline, size + size);
            abort_with_error("realloc()", p == NULL);
            tmp_firstline = p;
        }
    }

    *firstline = (char *) memory_alloc(strlen(tmp_firstline) + 1);                                                      /* Reallocation to reduce amount of memory used */
    if (*firstline != NULL)
        memcpy(*firstline, tmp_firstline, strlen(tmp_firstline));
    else {
        fprintf(stderr, "Error in realloc()\n");
        exit(EXIT_FAILURE);
    }

    *((*firstline) + strlen(tmp_firstline)) = '\0';

    free(tmp_firstline);

    return OK;
}

int parse_name(char *line, char **image_name) {

    size_t i = 0, j = 0;
    size_t size = strlen(line), image_preallocation = IMAGE_NAME_PREALLOCATION;
    char *p = (char *)memory_alloc(IMAGE_NAME_PREALLOCATION), *tmp;


    while (i < size) {

        if (line[i] == '/') {
            if (line[i + 1] == '?') {
                i = i + 2;

                for (; line[i] != ' '; i++) {
                    //*((*image_name) + j) = line[i];
                    p[j] = line[i];
                    j++;

                    if(j == IMAGE_NAME_PREALLOCATION) {
                        tmp = realloc(p, IMAGE_NAME_PREALLOCATION + IMAGE_NAME_PREALLOCATION);
                        abort_with_error("realloc()\n", p == NULL);
                        p = tmp;
                        image_preallocation += image_preallocation;
                    }

                }
                //*((*image_name) + j) = '\0';
                p[j] = '\0';
                break;

            } else if(line[i + 1] == ' ') {
                return EMPTY_PATH;
            } else {

                if (strstr(line, "favicon") != NULL) return ICON_REQUESTED;                                             /* If ? not present, try to check if the icon is requested*/

                else return MESSAGE_NOT_CORRECT;                                                                        /* If ? not present and the icon is not requested, the message is not correct*/

            }
        }

        i++;
    }

    *image_name = (char *) memory_alloc(strlen(p) + 1);                                                                 /* Reallocation to reduce amount of memory used */
    memcpy(*image_name, p, strlen(p));
    *((*image_name) + strlen(p)) = '\0';

    free(p);

    printf("IMAGE NAME %s\n", *image_name);

    return IMAGE_REQUESTED;

}

int get_accept_line(char *message, char **accept_line) {

    int i = 0;
    char *tmp = strstr(message, "Accept: ");
    size_t preallocation = 128;

    if (tmp == NULL) {
        fprintf(stderr, "Error in strstr(), Accept not present\n");
        return MESSAGE_NOT_CORRECT;
    }

    char *tmp_acceptline = (char *) memory_alloc(256 * sizeof(char)), *p;

    /* i+8 jumps Accept: characters */
    while (tmp[i + 8] != '\r') {
        tmp_acceptline[i] = tmp[i + 8];
        i++;

        if(i == preallocation) {
            p = realloc(tmp_acceptline, preallocation+preallocation);
            abort_with_error("realloc()\n", p == NULL);
            tmp_acceptline = p;
            preallocation += preallocation;
        }
    }
    tmp_acceptline[i] = '\0';

    *accept_line = memory_alloc(sizeof(char) * (strlen(tmp_acceptline) + 1));
    memcpy(*accept_line, tmp_acceptline, strlen(tmp_acceptline));
    *((*accept_line) + strlen(tmp_acceptline)) = '\0';

    printf("*acceptline %s\n", *accept_line);

    free(tmp_acceptline);

    return OK;
}

int get_user_agent(char *message, char **user_agent) {

    int i = 0;
    size_t preallocation = USERAGENT_PREALLOCATION;

    char *tmp = NULL, *tmp_user_agent = NULL;

    tmp = strstr(message, "User-Agent: ");
    if (!tmp) {
        tmp = strstr(message, "user-agent:");
    }

    tmp_user_agent = (char *)memory_alloc(USERAGENT_PREALLOCATION);

    /*i+8 jumps Accept: characters. It is always present User-agent line,
     * that prevents segmentation fault*/
    while (tmp[i + 12] != '\r') {

        tmp_user_agent[i] = tmp[i+12];
        i++;

        if (i == USERAGENT_PREALLOCATION) {
            char *p = realloc(tmp_user_agent, preallocation+preallocation);
            abort_with_error("realloc()\n", p == NULL);
            tmp_user_agent = p;
            preallocation += preallocation;
        }

    }
    tmp_user_agent[i] = '\0';

    *user_agent = memory_alloc(sizeof(char) * (strlen(tmp_user_agent) + 1));
    memcpy(*user_agent, tmp_user_agent, strlen(tmp_user_agent));
    *((*user_agent) + strlen(tmp_user_agent)) = '\0';

    free(tmp_user_agent);

    return OK;

}

void get_list_accept_image(char *accept_line, ImageNode **image_list) {

    size_t preallocation = 4, size = 0;
    unsigned int dim = 0;

    ImageNode *buf_im_list = NULL, *p;
    buf_im_list = (ImageNode *)memory_alloc(preallocation * sizeof(ImageNode));

    char *token1 = NULL;                                                                                                /* Token to split the string by commas */
    char *token2 = NULL;                                                                                                /* Token to split the string by semicolon */

    const char sep[2] = ",";
    const char sep2[2] = ";";

    unsigned int i = 0;                                                                                                 /* Iterator for the image_list array */

    token1 = strtok(accept_line, sep);

    while (token1 != NULL) {

        /* If in the token is found "text" or "application", it is discarded */
        if (strstr(token1, "text") != NULL || strstr(token1, "application") != NULL) {
            token1 = strtok(NULL, sep);
            continue;
        }

        buf_im_list[dim].type = strdup(token1);
        int idx = 0;
        while(idx < dim) {
            printf("bufimlist[dim] %s\n", buf_im_list[idx].type);
            idx++;
        }
        idx= 0;

        token1 = strtok(NULL, sep);                                                                                     /* Now NULL is used to work with the same string as stated in the man page */

        i++;
        dim++;

        if (dim == preallocation) {
            p = (ImageNode *) realloc(buf_im_list, preallocation + preallocation);
            abort_with_error("realloc()", p == NULL);
            buf_im_list = p;
        }
    }

    token1 = NULL;

    /*Once we got the tokens, we have to parse the quality factor*/
    i = 0;
    char *s;                                                                                                            /* Parameter in strtof()*/

    while (i < dim) {

        token1 = strtok(buf_im_list[i].type, sep2);
        token2 = strtok(NULL, sep2);

        if (token1 != NULL) {
            if (strstr(token1, "image/") != NULL && token2 == NULL) {
                if (strcmp(token1, "image/*") == 0) {
                    buf_im_list[i].extension = ALL_EXT;
                }
                if(strstr(token1, "jpg") != NULL)
                    buf_im_list[i].extension = JPG;
                if(strstr(token1, "jpeg") != NULL)
                    buf_im_list[i].extension = JPG;
                if(strstr(token1, "png") != NULL)
                    buf_im_list[i].extension = PNG;
                buf_im_list[i].q = 1.0;
                printf("quality %f\n", buf_im_list[i].q);
                i++;
                continue;
            } else if (strstr(token1, "*/*") != NULL) {
                if (token2 != NULL) {
                    errno = 0;
                    buf_im_list[i].q = strtof(token2 + 3, &s);
                    if(!buf_im_list[i].q)
                        buf_im_list[i].q = 0.8;
                    buf_im_list[i].extension = ALL_EXT;
                    i++;
                    continue;
                }
            }
        } else if (token2 == NULL) {
            i++;
            continue;
        }

        errno = 0;
        buf_im_list[i].q = strtof(token2 + 3, &s);
        if (errno != 0) {
            fprintf(stderr, "Error in strtof\n");
            buf_im_list[i].q = 0.8;
        }
        if(strstr(token1, "jpg") != NULL)
            buf_im_list[i].extension = JPG;
        if(strstr(token1, "jpeg") != NULL)
            buf_im_list[i].extension = JPG;
        if(strstr(token1, "png") != NULL)
            buf_im_list[i].extension = PNG;

        i++;

    }

    p = (ImageNode *)realloc(buf_im_list, dim * sizeof(ImageNode));
    int j = 0;
    while(j < dim) {
        p[j].type = buf_im_list[j].type;
        p[j].q = buf_im_list[j].q;
        j++;
    }

    *image_list = p;

    free(token1);

    //order_list(image_list);
}

void parse_command(char *first_line, int *cmd) {

    int i = 0;

    while(i < strlen(first_line)) {
        if(strncmp(first_line + i, "GET", 3) == 0) {
            *cmd = GET_CMD;
            return;
        } else if (strncmp(first_line + i, "HEAD", 4) == 0) {
            *cmd = HEAD_CMD;
            return;
        }
        i++;
    }

    *cmd = MESSAGE_NOT_CORRECT;
}

int parse_message(char *message, struct Request **request) {

    *request = create_request();
    int ret;

    char *first_line = NULL;
    struct image_node_t *list = NULL;

    ret = get_command_line(message, &first_line);
    if (ret == OK) {

        parse_command(first_line, &((*request)->cmd));

        ret = parse_name(first_line, &((*request)->image_name));
        if (ret == ICON_REQUESTED || ret == EMPTY_PATH) {
            (*request)->image_name = ICON_NAME;
            free(first_line);
            return ICON_REQUESTED;
        } else if (ret == MESSAGE_NOT_CORRECT) {
            free(first_line);
            return MESSAGE_NOT_CORRECT;
        }

        (*request)->ext = strrchr((*request)->image_name, '.');                                                         /* Extension of the image in the URL */

        free(first_line);

        char *accept_line = NULL;
        ret = get_accept_line(message, &accept_line);
        if(ret == MESSAGE_NOT_CORRECT)
            return MESSAGE_NOT_CORRECT;

        char *user_agent = NULL;
        ret = get_user_agent(message, &user_agent);
        (*request)->user_agent = user_agent;

        if(ret == OK) {

            get_list_accept_image(accept_line, &((*request)->image_list));

        }

    } else if (ret == REQUEST_TOO_LONG) {
        //Deal with it
    }

    return OK;
}

size_t build_message(off_t file_size, char *extension, char **msg) {

    size_t len = 0;

    char *char_size = convert_long_to_string(file_size);

    printf("in buiiiiiiild message %s\n", extension);

    memcpy(*msg, "HTTP/1.1 200 OK", strlen("HTTP/1.1 200 OK"));
    len += strlen("HTTP/1.1 200 OK");
    memcpy(*msg + len, "\r\n", strlen("\r\n"));
    len += strlen("\r\n");

    memcpy(*msg + len, "Server: 127.0.0.1", strlen("Server: 127.0.0.1"));
    len += strlen("Server: 127.0.0.1");
    memcpy(*msg + len, "\r\n", strlen("\r\n"));
    len += strlen("\r\n");

    memcpy(*msg + len, "Content-Type: image/", strlen("content-type: image/"));
    len += strlen("Content-Type: image/");
    //*msg = catenate_strings(tmp_msg, ext);
    /*while (i < strlen(ext)) {
        *((*msg) + len + i) = ext[i];
        i++;
    }
    len += strlen(ext);*/
    memcpy(*msg + len, "jpeg", strlen("jpeg"));
    len += strlen("jpeg");
    memcpy(*msg + len, "\r\n", strlen("\r\n"));
    len += strlen("\r\n");

    memcpy(*msg + len, "Content-Transfer-Encoding: binary", strlen("Content-Transfer-Encoding: binary"));
    len += strlen("Content-Transfer-Encoding: binary");
    memcpy(*msg + len, "\r\n", strlen("\r\n"));
    len += strlen("\r\n");

    memcpy(*msg + len, "Content-Length: ", strlen("Content-Length: "));
    len += strlen("Content-Length: ");
    memcpy(*msg + len, char_size, strlen(char_size));
    len += strlen(char_size);
    memcpy(*msg + len, "\r\n", strlen("\r\n"));
    len += strlen("\r\n");

    memcpy(*msg + len, "Connection: Keep-Alive", strlen("Connection: Keep-Alive"));
    len += strlen("Connection: Keep-Alive");
    memcpy(*msg + len, "\r\n\r\n", strlen("\r\n\r\n"));
    len += strlen("\r\n\r\n");

    printf("message built %s\n\n", *msg);

    return len;
}

int send_image(int conn_sd, struct image_t *image, int cmd) {

    char *msg = NULL;
    msg = (char *) memory_alloc((size_t) 512 * sizeof(char));
    msg[511] = '\0';

    size_t len_header = build_message(image->file_size, image->ext, &msg);
    ssize_t s = 0;
    size_t sent = 0;

    /************ Send headers **********************/
    while (sent < len_header) {

        s = send(conn_sd, msg + sent, strlen(msg), MSG_MORE);

        if (s == -1) {
            fprintf(stderr, "Error in send()\n");
            return ERROR_SENDING_MESSAGE;
        }

        sent += s;
    }
    /********************************************/

    if(cmd == GET_CMD) {
        int retry = 5;

        ssize_t v = 0;

        while (retry > 0) {
            v = sendfile(conn_sd, image->fd, NULL, (size_t) image->file_size);
            if (v == -1) {
                fprintf(stderr, "Error in sendfile()\n");
                return ERROR_SENDING_MESSAGE;
            } else if (v != image->file_size) {
                fprintf(stderr, "Image sent not correctly\n");
                retry--;
                continue;
            } else if (v == image->file_size)
                break;
        }
    }

    return OK;
}