//
// Created by federico on 31/10/17.
//

#ifndef IMAGEAPP_HTTPRESPONSE_H
#define IMAGEAPP_HTTPRESPONSE_H

char bad_request[] = "<!DOCTYPE HTML PUBLIC>\n"
        "<html>\n"
        "\n"
        "<head>\n"
        "   <title>400 Bad Request</title>\n"
        "</head>\n"
        "\n"
        "<body>\n"
        "   <h1>Bad Request</h1>\n"
        "   <p>Your browser sent a request that this server could not understand.<p>\n"
        "   <p>The request line contained invalid characters following the protocol string.<p>\n"
        "</body>\n"
        "\n"
        "</html>";

char service_unavailable[] = "<!DOCTYPE HTML PUBLIC>\n"
        "<html>\n"
        "\n"
        "<head>\n"
        "   <title>503 Service Unavailable</title>\n"
        "</head>\n"
        "\n"
        "<body>\n"
        "   <h1>Service unavailablet</h1>\n"
        "   <p>Your browser sent a request that this server could not satisfy.<p>\n"
        "   <p>Please retry in a few seconds<p>\n"
        "</body>\n"
        "\n"
        "</html>";

/*Other responses*/

#endif //IMAGEAPP_HTTPRESPONSE_H
