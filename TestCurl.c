//
// Created by federico on 19/10/17.
//

#include <stdio.h>
#include <curl/curl.h>

int main(void)
{
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if(curl) {
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

        if(CURLE_OK == res) {
            char *ct;
            /* ask for the content-type */
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);

            if((CURLE_OK == res) && ct)
                printf("We received Content-Type: %s\n", ct);
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    return 0;
}