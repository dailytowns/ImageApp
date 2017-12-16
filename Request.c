#include <errno.h>
#include <string.h>
#include <sys/sendfile.h>

#include "include/HtmlResponse.h"
#include "include/Request.h"
#include "include/Utils.h"

void destroy_request(struct request_t *request, int status_r) {

    free(request->user_agent);
    free(request->image_list);
    if(status_r != MESSAGE_NOT_CORRECT)
        free(request->image_name);
    free(request->cache_name);
    free(request);

}

int receive_request(struct thread_data *td, int idx) {

    ssize_t v = 0, received = 0;
    size_t tot = HTTP_MESSAGE_SIZE;
    int error;
    int retry = 2;

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
            } else if ((error == ECONNRESET) || (errno == EBADF)) {
                return EMPTY_MESSAGE;
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
                return EMPTY_MESSAGE;
            } else {
                fprintf(stderr, "Error in recv(), error = %s\n", strerror(error));
            }

        }
    }

}

int get_command_line(const char *message, char **firstline) {

    size_t i = 0;
    size_t size = 128;

    /*char *tmp_firstline = NULL;
    tmp_firstline = memory_alloc(size * sizeof(char));*/

    char tmp_firstline[512];

    while (1) {
        if (message[i] != '\r') {
            tmp_firstline[i] = message[i];
        } else {
            tmp_firstline[i] = '\0';                                                                                    /* '\0' is the end of string. This assignment deals with SIGSEGV strcpy()s or in strlen()*/
            break;
        }
        i++;

        /*if (i ==
            size) {                                                                                                 /* Reallocation in case of a name too long */
            /*if (size == (size + size))
                return REQUEST_TOO_LONG;
            size += size;
            char *p = realloc(tmp_firstline, size + size);
            abort_with_error("realloc()", p == NULL);
            tmp_firstline = p;
        }*/
    }

    *firstline = memory_alloc(i+1);                                                                              /* Reallocation to reduce amount of memory used */

    if (*firstline != NULL)
        memcpy(*firstline, tmp_firstline, i);
    else {
        fprintf(stderr, "Error in realloc()\n");
        exit(EXIT_FAILURE);
    }

    *((*firstline) + i) = '\0';

    return OK;
}

int parse_name(char *line, char **image_name, char **ext) {

    size_t i = 0, j = 0;
    size_t size = strlen(line);
    size_t image_preallocation = IMAGE_NAME_PREALLOCATION;
    char *p = memory_alloc(IMAGE_NAME_PREALLOCATION), *tmp;
    char *e = memory_alloc(5);

    while (i < size) {

        if (line[i] == '/') {
            if (line[i + 1] == '?') {
                i = i + 2;

                for (; (line[i] != '.') && (line[i] != ' '); i++) {
                    //*((*image_name) + j) = line[i];
                    p[j] = line[i];
                    j++;

                    if (j == IMAGE_NAME_PREALLOCATION) {
                        tmp = realloc(p, IMAGE_NAME_PREALLOCATION + IMAGE_NAME_PREALLOCATION);
                        abort_with_error("realloc()\n", p == NULL);
                        p = tmp;
                        image_preallocation += image_preallocation;
                    }

                }
                //*((*image_name) + j) = '\0';
                if (line[i] == ' ')
                    return MESSAGE_NOT_CORRECT;

                p[j] = '\0';

                j = 0;
                for (; line[i] != ' '; i++) {
                    e[j] = line[i];
                    j++;
                }

                e[4] = '\0';

                break;

            } else if (line[i + 1] == ' ') {
                return EMPTY_PATH;
            } else {

                if (strstr(line, "favicon") != NULL)
                    return ICON_REQUESTED;                                             /* If ? not present, try to check if the icon is requested*/

                else return MESSAGE_NOT_CORRECT;                                                                        /* If ? not present and the icon is not requested, the message is not correct*/

            }
        }

        i++;
    }

    *image_name = memory_alloc(strlen(p) +
                                        1);                                                                 /* Reallocation to reduce amount of memory used */
    memcpy(*image_name, p, strlen(p));
    *((*image_name) + strlen(p)) = '\0';

    *ext = memory_alloc(strlen(e) + 1);
    memcpy(*ext, e, strlen(e));
    *((*ext) + strlen(e)) = '\0';

    free(p);
    free(e);

    //printf("IMAGE NAME %s\n", *image_name);

    return IMAGE_REQUESTED;

}

int get_accept_line(char *message, char **accept_line) {

    size_t i = 0, preallocation = 128;
    char tmp[preallocation];
    memcpy(tmp, strstr(message, "Accept: "), preallocation);
    tmp[preallocation - 1] = '\0';

    char tmp_acceptline[preallocation];

    /* i+8 jumps Accept: characters */
    while (i < (preallocation - 8)) {

        if(tmp[i + 8] == '\r') {
            tmp_acceptline[i] = '\0';
            break;
        }

        tmp_acceptline[i] = tmp[i + 8];
        i++;

    }

    *accept_line = memory_alloc(i+1);
    memcpy(*accept_line, tmp_acceptline, i);
    *((*accept_line) + i) = '\0';

    return OK;
}

int get_user_agent(char *message, char **user_agent) {

    size_t i = 0;
    size_t preallocation = USERAGENT_PREALLOCATION;

    char tmp[256];
    memcpy(tmp, strstr(message, "User-Agent: "), preallocation);
    tmp[255] = '\0';

    char tmp_user_agent[256];

    /*i+8 jumps Accept: characters. It is always present User-Agent line,
     * that prevents segmentation fault*/
    while (tmp[i + 12] != '\r') {

        tmp_user_agent[i] = tmp[i + 12];
        i++;

    }
    tmp_user_agent[i] = '\0';

    *user_agent = memory_alloc(i+1);
    memcpy(*user_agent, tmp_user_agent, i);
    *((*user_agent) + i) = '\0';

    return OK;

}

void get_list_accept_image(char *accept_line, ImageNode **image_list) {

    size_t preallocation = 4;
    unsigned int dim = 0;

    ImageNode *buf_im_list = NULL, *p;
    buf_im_list = memory_alloc(preallocation * sizeof(ImageNode));

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

        token1 = strtok(NULL,
                        sep);                                                                                     /* Now NULL is used to work with the same string as stated in the man page */

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
                    buf_im_list[i].q = 1.0;
                    //printf("quality %f\n", buf_im_list[i].q);
                    i++;
                    continue;
                }

                if (strstr(token1, "jpg") != NULL)
                    buf_im_list[i].extension = JPG;
                else if (strstr(token1, "jpeg") != NULL)
                    buf_im_list[i].extension = JPG;
                else if (strstr(token1, "png") != NULL)
                    buf_im_list[i].extension = PNG;
                else if (strstr(token1, "webp") != NULL)
                    buf_im_list[i].extension = WEBP;
                else if (strstr(token1, "jxr") != NULL)
                    buf_im_list[i].extension = JXR;
                buf_im_list[i].q = 1.0;

                i++;
                continue;

            } else if (strstr(token1, "*/*") != NULL) {

                if (token2 != NULL) {

                    errno = 0;
                    buf_im_list[i].q = strtof(token2 + 3, &s);
                    if (!buf_im_list[i].q)
                        buf_im_list[i].q = 0.8;

                }

                buf_im_list[i].extension = ALL_EXT;
                i++;
                continue;
            }
        }

        if(token2 != NULL) {
            errno = 0;
            buf_im_list[i].q = strtof(token2 + 3, &s);
            if (errno != 0) {
                fprintf(stderr, "Error in strtof\n");
                buf_im_list[i].q = 0.8;
            }
        }

        if (strstr(token1, "jpg") != NULL)
            buf_im_list[i].extension = JPG;
        if (strstr(token1, "jpeg") != NULL)
            buf_im_list[i].extension = JPG;
        if (strstr(token1, "png") != NULL)
            buf_im_list[i].extension = PNG;

        i++;

    }

    *image_list = (ImageNode *) realloc(buf_im_list, dim * sizeof(ImageNode));
    abort_with_error("realloc()", *image_list == NULL);

    free(token1);                                                                                                       /* Always free memory */

}

void parse_command(char *first_line, int *cmd) {

    int i = 0;

    while (i < strlen(first_line)) {
        if (strncmp(first_line + i, "GET", 3) == 0) {
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

int parse_request(char *message, struct request_t **request) {

    *request = memory_alloc(sizeof(struct request_t));                                                                  /* Allocated the request */
    int ret;

    char *first_line = NULL;

    ret = get_command_line(message, &first_line);
    if (ret == OK) {

        parse_command(first_line, &((*request)->cmd));

        ret = parse_name(first_line, &((*request)->image_name), &((*request)->ext));

        if (ret == ICON_REQUESTED || ret == EMPTY_PATH) {
            (*request)->image_name = ICON_NAME;
            free(first_line);
            return ICON_REQUESTED;
        } else if (ret == MESSAGE_NOT_CORRECT) {
            (*request)->image_name = "";
            free(first_line);
            return MESSAGE_NOT_CORRECT;
        }

        free(first_line);

        char *accept_line = NULL;
        ret = get_accept_line(message, &accept_line);
        if (ret == MESSAGE_NOT_CORRECT)
            return MESSAGE_NOT_CORRECT;

        char *user_agent = NULL;
        ret = get_user_agent(message, &user_agent);
        (*request)->user_agent = user_agent;

        if (ret == OK) {

            get_list_accept_image(accept_line, &((*request)->image_list));

        }

        free(accept_line);

    }

    return REQUEST_RECEIVED;
}

ssize_t send_file_fd(int conn_sd, int fd, off_t size) {

    ssize_t v;
    off_t offset = 0;
    size_t len = 0;

    while (len < size) {

        errno = 0;
        v = sendfile(conn_sd, fd, &offset, size - len);
        //v = read_block(fd, buf, 4096);
        //v = write_block(conn_sd, buf, 4096);

        if (v == -1) {

            if(errno == EPIPE) {
                return CONNECTION_CLOSED;
            }

            fprintf(stderr, "len=%ld conn_sd=%d\n", len, conn_sd);
            return ERROR_SENDING_MESSAGE;

        }

        len += v;
        offset += v;

    }

    return len;

}

int send_image(int conn_sd, struct image_t *image,
               int cmd) {

    char msg[512];

    size_t len_header = build_message(image->file_size, image->ext, msg, image, NULL);
    ssize_t s = 0;
    size_t sent = 0;

    /************ Send headers **********************/
    while (sent < len_header) {

        s = send(conn_sd, msg + sent, strlen(msg), 0);

        if (s == -1) {
            fprintf(stderr, "Error in send()\n");
            return ERROR_SENDING_MESSAGE;
        }

        sent += s;
    }
    /********************************************/

    /* sendfile() leaves memory allocated, replaced with read-write loop
     */
    if (cmd == GET_CMD) {
        int retry = 5;

        ssize_t v = 0;

        while (retry > 0) {
            v = send_file_fd(conn_sd, image->fd, (size_t) image->file_size);
            if (v == ERROR_SENDING_MESSAGE) {

                fprintf(stderr, "Error in sendfile(), error %s\n", strerror(errno));
                return ERROR_SENDING_MESSAGE;

            } else if (v == CONNECTION_CLOSED) {
                return CONNECTION_CLOSED;
            } else if (v != image->file_size) {

                fprintf(stderr, "Image sent not correctly, size %ld retrying\n", image->file_size);
                retry--;
                continue;


            } else if (v == image->file_size)
                break;

            if (retry == 0) {
                fprintf(stderr, "Image sent not correctly\n");
            }
        }

        return OK;

    }
}

int send_bad_request(int conn_sd) {

        //char *msg = NULL;
        //msg = memory_alloc((size_t) 512 * sizeof(char));
        //msg[511] = '\0';

        char msg[512];

        //char *str_bad_request = memory_alloc(strlen(bad_request) + 1);
        //memcpy(str_bad_request, bad_request, strlen(bad_request));
        //str_bad_request[strlen(bad_request)] = '\0';
        //size_t len_bad = strlen(str_bad_request) + 1;
        size_t len_bad = strlen(bad_request) + 1;


        size_t len_header = build_message(len_bad, "text/html", msg, NULL, bad_request);
        //size_t len_header = build_message(len_bad, "text/html", msg, NULL, str_bad_request);
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

        int retry = 5;

        ssize_t v, w = 0;

        while (retry > 0) {

            while (len_bad - w > 0) {
                v = write_block(conn_sd, bad_request + w, len_bad - w);
                if (v == -1 && errno != EAGAIN) {
                    fprintf(stderr, "Error sending image\n");
                    return ERROR_SENDING_MESSAGE;
                }

                w += v;
            }

            if ((w != len_bad) && (retry != 0)) {
                retry--;
                continue;
            } else if ((w != len_bad) && (retry == 0))
                return ERROR_SENDING_MESSAGE;
            else if (w == len_bad)
                break;

        }

        //free(msg);
        //free(str_bad_request);
}

int send_service_unavailable(int conn_sd) {

        char msg[512];

        //char *str_service_unavailable = memory_alloc(strlen(service_unavailable) + 1);
        //memcpy(str_service_unavailable, service_unavailable, strlen(service_unavailable));
        //str_service_unavailable[strlen(service_unavailable)] = '\0';
        //size_t len_ser = strlen(service_unavailable) + 1;
        size_t len_ser = strlen(service_unavailable) + 1;

        //size_t len_header = build_message(len_ser, "text/html", msg, NULL, str_service_unavailable);
        size_t len_header = build_message(len_ser, "text/html", msg, NULL, service_unavailable);
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

        int retry = 5;

        ssize_t v, w = 0;

        while (retry > 0) {

            while (len_ser - w > 0) {
                v = write_block(conn_sd, service_unavailable + w, len_ser - w);
                if (v == -1 && errno != EAGAIN) {
                    fprintf(stderr, "Error sending image\n");
                    return ERROR_SENDING_MESSAGE;
                }

                w += v;
            }

            if ((w != len_ser) && (retry != 0)) {
                retry--;
                continue;
            } else if ((w != len_ser) && (retry == 0))
                return ERROR_SENDING_MESSAGE;
            else if (w == len_ser)
                break;

        }

        //free(msg);
        //free(str_service_unavailable);

}

