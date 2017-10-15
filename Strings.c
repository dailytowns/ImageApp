//
// Created by federico on 14/10/17.
//

#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "include/Strings.h"
#include "include/Utils.h"

char *get_filename_ext(char *filename) {

    char *tmp = memory_alloc(5 * sizeof(char));
    memcpy(tmp, strrchr(filename, '.'), 4);
    if (!tmp || tmp == filename) return NULL;

    tmp[4] = '\0';
    return tmp;
}


int remove_extension(char *name) {

    if (name == NULL)
        return 0;

    char *first_occ = strchr(name, '.');

    if (first_occ != NULL)
        return (int) (first_occ - name) + 1;
    else
        return -1;
}

void build_image_name_cache(char **buf, char *name, float quality, int width, int height) {

    int v = remove_extension(name);
    char *filename = NULL;
    if (v > 0) {
        filename = (char *) memory_alloc(v * sizeof(char));
        memcpy(filename, name, v);
        filename[v - 1] = '\0';
    } else
        filename = name;

    *buf = memory_alloc((v +
                         sizeof(quality) +
                         sizeof(height) +
                         sizeof(width)) * sizeof(char));
    sprintf(*buf, "%s%f%d%d", filename, quality, width, height);

    free(filename);

    printf("DOPO SPRINTF %s\n", *buf);

}

char *catenate_strings(char *s1, char *s2) {

    printf("INCATENATE\n");

    size_t size_s1 = strlen(s1);
    size_t size_s2 = strlen(s2);

    char *buf = NULL;
    buf = (char *) memory_alloc(((size_s1 + size_s2 + 1)) * sizeof(char));

    memcpy(buf, s1, strlen(s1));
    memcpy(buf + size_s1, s2, size_s2);
    memset(buf + size_s1 + size_s2, 0, strlen(buf) - strlen(s1) - size_s2);
    *(buf + size_s1 + size_s2) = '\0';
    printf("INCATENATE\n");

    return buf;
}

char *convert_long_to_string (long size) {

    long value = (long) size;
    long tmp_value = value;

    int count_cipher = 0;
    while (tmp_value > 0) {
        tmp_value = tmp_value / 10;
        count_cipher++;
    }

    int i = 1;
    char *str_number = (char *) memory_alloc((size_t) count_cipher + 1);
    while (count_cipher - i >= 0 && value > 0) {
        int tmp = (char) (value % 10);
        str_number[count_cipher - i] = (char) ((char) tmp + '0');
        value /= 10;
        i++;
    }
    str_number[count_cipher] = '\0';

    printf("\nCONVERSIONE %s\n", str_number);

    return str_number;
}
