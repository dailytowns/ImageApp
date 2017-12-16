#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "include/HandleImage.h"
#include "include/Strings.h"
#include "include/Utils.h"

/*The double pointer for buf parameter has been preserved because the memory is allocated here
 * */
void build_image_name_cache(char **buf, char *name, float quality, int width, int height, int colors) {

    char *str_quality = convert_float_to_string(quality);
    char *str_width = convert_int_to_string(width);
    char *str_height = convert_int_to_string(height);
    char *str_colors = convert_int_to_string(colors);

    size_t len = 0;

    *buf = memory_alloc(strlen(name) +
                                strlen(str_quality) +
                                strlen(str_height) +
                                strlen(str_colors) +
                                strlen(str_width) + 1);

    memcpy(*buf, name, strlen(name));
    len += strlen(name);
    memcpy(*buf + len, str_quality, strlen(str_quality));
    len += strlen(str_quality);
    memcpy(*buf + len, str_width, strlen(str_width));
    len += strlen(str_width);
    memcpy(*buf + len, str_height, strlen(str_height));
    len += strlen(str_height);
    memcpy(*buf + len, str_colors, strlen(str_colors));
    len += strlen(str_colors);
    *((*buf) + len) = '\0';

    free(str_quality);
    free(str_height);
    free(str_colors);
    free(str_width);

}

char *catenate_strings(char *s1, char *s2) {                                                                            /* Calling this function avoids preallocation of the destination string */

    char *buf = memory_alloc(strlen(s1) + strlen(s2) + 1);

    strcat(buf, s1);
    strcat(buf + strlen(s1), s2);
    buf[strlen(s1) + strlen(s2)] = '\0';

    return buf;
}

char *convert_int_to_string(int size) {

    int value = size;
    int tmp_value = value;

    size_t count_cipher = 0;
    while (tmp_value > 0) {
        tmp_value = tmp_value / 10;
        count_cipher++;
    }

    int i = 1;

    char *str_number = memory_alloc((size_t) (count_cipher + 1));
    while ((count_cipher - i >= 0) && (value > 0)) {
        int tmp = (char) (value % 10);
        str_number[count_cipher - i] = (char) ((char) tmp + '0');
        value /= 10;
        i++;
    }
    str_number[count_cipher] = '\0';

    return str_number;
}

char *convert_long_to_string(long value) {

    long tmp_value = value;

    size_t count_cipher = 0;
    while (tmp_value > 0) {
        tmp_value = tmp_value / 10;
        count_cipher++;
    }

    int i = 1;
    char *str_number = memory_alloc(count_cipher + 1);
    while (count_cipher - i >= 0 && value > 0) {
        int tmp = (char) (value % 10);
        str_number[count_cipher - i] = (char) ((char) tmp + '0');
        value /= 10;
        i++;
    }

    str_number[count_cipher] = '\0';

    return str_number;
}

size_t build_message(off_t file_size, char *mime_type, char *msg, struct image_t *image, char *msg_error) {

    size_t len = 0;

    char *char_size = convert_long_to_string(file_size);

    if (image) {

        memcpy(msg, "HTTP/1.1 200 OK", strlen("HTTP/1.1 200 OK"));
        len += strlen("HTTP/1.1 200 OK");
        memcpy(msg + len, "\r\n", strlen("\r\n"));
        len += strlen("\r\n");

        memcpy(msg + len, "Server: 127.0.0.1", strlen("Server: 127.0.0.1"));
        len += strlen("Server: 127.0.0.1");
        memcpy(msg + len, "\r\n", strlen("\r\n"));
        len += strlen("\r\n");

        memcpy(msg + len, "Content-Type: image/", strlen("content-type: image/"));
        len += strlen("Content-Type: image/");
        memcpy(msg + len, "jpeg", strlen("jpeg"));
        len += strlen("jpeg");
        memcpy(msg + len, "\r\n", strlen("\r\n"));
        len += strlen("\r\n");

        memcpy(msg + len, "Content-Transfer-Encoding: binary", strlen("Content-Transfer-Encoding: binary"));
        len += strlen("Content-Transfer-Encoding: binary");
        memcpy(msg + len, "\r\n", strlen("\r\n"));
        len += strlen("\r\n");

        memcpy(msg + len, "Content-Length: ", strlen("Content-Length: "));
        len += strlen("Content-Length: ");
        memcpy(msg + len, char_size, strlen(char_size));
        len += strlen(char_size);
        memcpy(msg + len, "\r\n", strlen("\r\n"));
        len += strlen("\r\n");

        memcpy(msg + len, "Keep-Alive: timeout=5, max=10", strlen("Keep-Alive: timeout=5, max=10"));
        len += strlen("Keep-Alive: timeout=5, max=10");
        memcpy(msg + len, "\r\n\r\n", strlen("\r\n\r\n"));
        len += strlen("\r\n\r\n");

    } else {

        if (file_size == (strlen(msg_error) + 1)) {
            memcpy(msg, "HTTP/1.1 400 Bad Request", strlen("HTTP/1.1 400 Bad Request"));
            len += strlen("HTTP/1.1 400 Bad Request");
            memcpy(msg + len, "\r\n", strlen("\r\n"));
            len += strlen("\r\n");

            memcpy(msg + len, "Server: 127.0.0.1", strlen("Server: 127.0.0.1"));
            len += strlen("Server: 127.0.0.1");
            memcpy(msg + len, "\r\n", strlen("\r\n"));
            len += strlen("\r\n");

            memcpy(msg + len, "Content-Type: text/html", strlen("Content-Type: text/html"));
            len += strlen("Content-Type: text/html");
            memcpy(msg + len, "\r\n", strlen("\r\n"));
            len += strlen("\r\n");

            memcpy(msg + len, "Content-Length: ", strlen("Content-Length: "));
            len += strlen("Content-Length: ");
            memcpy(msg + len, char_size, strlen(char_size));
            len += strlen(char_size);
            memcpy(msg + len, "\r\n", strlen("\r\n"));
            len += strlen("\r\n");

            memcpy(msg + len, "Connection: Closed", strlen("Connection: Closed"));
            len += strlen("Connection: Closed");
            memcpy(msg + len, "\r\n\r\n", strlen("\r\n\r\n"));
            len += strlen("\r\n\r\n");

        } else if (file_size == (strlen(msg_error) + 1)) {

            memcpy(msg, "HTTP/1.1 503 Service Unavailable", strlen("HTTP/1.1 503 Service Unavailable"));
            len += strlen("HTTP/1.1 503 Service Unavailable");
            memcpy(msg + len, "\r\n", strlen("\r\n"));
            len += strlen("\r\n");

            memcpy(msg + len, "Server: 127.0.0.1", strlen("Server: 127.0.0.1"));
            len += strlen("Server: 127.0.0.1");
            memcpy(msg + len, "\r\n", strlen("\r\n"));
            len += strlen("\r\n");

            memcpy(msg + len, "Content-Type: text/html", strlen("Content-Type: text/html"));
            len += strlen("Content-Type: text/html");
            memcpy(msg + len, "\r\n", strlen("\r\n"));
            len += strlen("\r\n");

            memcpy(msg + len, "Content-Length: ", strlen("Content-Length: "));
            len += strlen("Content-Length: ");
            memcpy(msg + len, char_size, strlen(char_size));
            len += strlen(char_size);
            memcpy(msg + len, "\r\n", strlen("\r\n"));
            len += strlen("\r\n");

            memcpy(msg + len, "Connection: Closed", strlen("Connection: Closed"));
            len += strlen("Connection: Closed");
            memcpy(msg + len, "\r\n\r\n", strlen("\r\n\r\n"));
            len += strlen("\r\n\r\n");

        }

    }

    msg[len] = '\0';
    free(char_size);
    return len;
}

char *convert_float_to_string(float n)
{

    int integer = (int)n;
    float mantissa = n - (float)integer;
    char *res = memory_alloc(11);                                                                                             /*0.(eight digits after decimal point)*/

    // convert integer part to string
    if(integer == 0)
        res[0] = '0';
    else {
        memcpy(res, "1.00000000", 10);
        res[10] = '\0';
        return res;
    }

    res[1] = '.';  // add dot

    mantissa = (int)(mantissa * pow(10, 8));

    char *str_dec = convert_int_to_string((int)mantissa);
    memcpy(res + 2, str_dec, 8);

    free(str_dec);
    res[10] = '\0';
    return res;
}