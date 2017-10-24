//
// Created by federico on 19/10/17.
//

#include <stdio.h>
#include <curl/curl.h>
#include <pthread.h>
#include <stdlib.h>

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

void *job(void *arg) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *chunk = NULL;

        /* Remove a header curl would otherwise add by itself */
        chunk = curl_slist_append(chunk, "Accept: image/jpeg; q=0.5, text/plain, image/png; q=0.6");

        /* Modify a header curl otherwise adds differently */
        chunk = curl_slist_append(chunk, "Host: localhost");

        /* Modify a header curl otherwise adds differently */
        chunk = curl_slist_append(chunk, "User-Agent: DO_NOT_MATCH_NOKIA_SERIES40");

        /* Modify a header curl otherwise adds differently */
        chunk = curl_slist_append(chunk, "Connection: keep-alive");

        /* set our custom set of headers */
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

        curl_easy_setopt(curl, CURLOPT_URL, "localhost:5193/?Blue-Water.jpg");

        res = curl_easy_perform(curl);

        if (CURLE_OK == res) {
            char *ct;
            /* ask for the content-type */
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);

            int size;
            curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &size);

            if ((CURLE_OK == res) && ct) {
                printf("We received Content-Type: %s\n", ct);
                printf("File size: %d\n", size);
            }
        }

        FILE *fp;

        char outfilename[FILENAME_MAX] = "./filecurl";
        curl = curl_easy_init();
        if (curl) {
            fp = fopen(outfilename, "wb");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            res = curl_easy_perform(curl);
            /* always cleanup */
            curl_easy_cleanup(curl);
            fclose(fp);
        }

        return 0;
    }
}

int main() {

    int i=0;
    pthread_t tid[10];

    while(i < 50) {
        if(pthread_create(tid+i, NULL, job, NULL) != 0) {
            fprintf(stderr, "Error in ptheoifvn\n");
        }
        i++;
    }

    int j = 0;
    while(j<10) {
        if(pthread_join(tid[j], NULL) != 0) {
            break;
        }
        j++;
    }

    return EXIT_SUCCESS;
}

