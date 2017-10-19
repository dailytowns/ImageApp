//
// Created by federico on 17/10/17.
//

#include <stdio.h>
#include <mysql.h>
#include <libxml/parser.h>
#include <stdio.h>
#include <libxml/tree.h>
#include <string.h>
#include <sys/types.h>


#include "include/Utils.h"
/*
 *To compile this file using gcc you can type
 *gcc `xml2-config --cflags --libs` -o xmlexample libxml2-example.c
 */

/**
 * populate_db:
 * @param a_node Root of the xml file
 * @param conn Connection to the database
 *
 * Inserts in the database the devices retrieved in the xml file
 *
 */
void populate_db(xmlNode *a_node, MYSQL *conn) {
    //xmlNode *cur_node = NULL;
    //xmlNode *iterator = NULL;
    //xmlNode *group_iterator = NULL;

    xmlNodePtr cur_node = NULL;
    xmlNodePtr iterator = NULL;
    xmlNodePtr group_iterator = NULL;

    int image_width, resolution_width, image_height, resolution_height;

    char *p;

    char *id, *user_agent;
    int height = 0, width = 0, png = 0, jpeg = 0, tiff = 0, colors = 0;
    char query[512];
    bzero(query, 512);

    cur_node = a_node->children;

    while (cur_node != NULL) {

        //if (cur_node->type == XML_ELEMENT_NODE) {
        printf("node type: Element, name: %s\n", cur_node->name);

        if (strncmp(cur_node->name, "devices", 7) == 0) {

            cur_node = cur_node->children;
            printf("IN STRCMP: cur_node name %s, next name %s\n", cur_node->name, (cur_node->next)->name);

            int i = 0;

            //tutti i device
            while (cur_node->next != NULL) {

                //printf("%d %s\n", ++i, cur_node->name);

                if (strncmp((char *) cur_node->name, "device", 6) == 0) {

                    id = xmlGetProp(cur_node, "id");
                    user_agent = xmlGetProp(cur_node, "user_agent");

                    //printf("ID E USERAGENT %s %s\n", id, user_agent);

                    if (cur_node->children != NULL)
                        iterator = cur_node->children;
                    else {
                        sprintf(query, "INSERT INTO device values ('%s', '%s', 0, 0, 0, 0, 0, 0);", id, user_agent);
                        if (mysql_query(conn, query)) {
                            fprintf(stderr, "%s\n", user_agent);
                            abort_with_error("mysql_query()", 1);
                        }
                        bzero(query, 512);
                        cur_node = cur_node->next;
                        continue;
                    }

                    /*******************************/

                    /******************************/

                    printf("%s ITERATOR NAME\n", iterator->name);
                    //IL FOR CICLA BENE
                    //iterator cicla su group
                    while (iterator != NULL) {
                        //printf("ITERATOR %s\n", iterator->name);
                        i++;
                        if (strncmp(iterator->name, "group", 5) == 0) {

                            if (xmlStrcmp((char *) xmlGetProp(iterator, "id"), "display") == 0) {
                                group_iterator = iterator->children;
                                image_height = 0, resolution_height = 0, image_width = 0, resolution_width = 0;
                                while (group_iterator->next != NULL) {

                                    if (xmlStrcmp((char *) xmlGetProp(group_iterator, "name"), "max_image_width") ==
                                        0) {
                                        image_width = strtol(xmlGetProp(group_iterator, "value"), &p, 0);
                                    } else {
                                        if (xmlStrcmp((char *) xmlGetProp(group_iterator, "name"),
                                                      "resolution_width") ==
                                            0) {
                                            resolution_width = strtol(xmlGetProp(group_iterator, "value"), &p, 0);
                                        }
                                    }

                                    if (xmlStrcmp((char *) xmlGetProp(group_iterator, "name"),
                                                  "max_image_height") == 0) {
                                        image_height = strtol(xmlGetProp(group_iterator, "value"), &p, 0);
                                    } else {
                                        if (xmlStrcmp((char *) xmlGetProp(group_iterator, "name"),
                                                      "resolution_height") == 0) {
                                            resolution_height = strtol(xmlGetProp(group_iterator, "value"), &p, 0);
                                        }
                                    }
                                    group_iterator = group_iterator->next;
                                }

                                if (image_height && resolution_height)
                                    height = image_height;
                                else
                                    height = resolution_height;

                                if (image_width && resolution_width)
                                    width = image_width;
                                else
                                    width = resolution_width;

                                printf("HEIGHT E WIDTH %d %d \n", height, width);

                            } else {
                                if (xmlStrcmp((char *) xmlGetProp(iterator, "id"), "image_format") == 0) {
                                    group_iterator = iterator->children;
                                    while (group_iterator->next != NULL) {

                                        if (xmlStrcmp(xmlGetProp(group_iterator, "name"), "jpg") == 0) {
                                            jpeg = (xmlStrcmp(xmlGetProp(group_iterator, "value"), "true")) ? 0 : 1;
                                        } else if (xmlStrcmp(xmlGetProp(group_iterator, "name"), "png") == 0) {
                                            png = (xmlStrcmp(xmlGetProp(group_iterator, "value"), "true")) ? 0 : 1;
                                        } else if (xmlStrcmp(xmlGetProp(group_iterator, "name"), "tiff") == 0) {
                                            tiff = (xmlStrcmp(xmlGetProp(group_iterator, "value"), "true")) ? 0 : 1;
                                        } else if (xmlStrcmp(xmlGetProp(group_iterator, "name"), "colors") == 0) {
                                            colors = strtol(xmlGetProp(group_iterator, "value"), &p, 0);
                                        }

                                        group_iterator = group_iterator->next;
                                    }

                                    printf("jpg png tiff colors %d %d %d %d\n", jpeg, png, tiff, colors);

                                }
                            }


                        }
                        iterator = iterator->next;
                    }

                    sprintf(query, "INSERT INTO device values ('%s','%s', %d, %d, %d, %d, %d, %d)",
                            id, user_agent, height, width, png, jpeg, tiff, colors);
                    if (mysql_query(conn, query)) {
                        fprintf(stderr, "%s\n", user_agent);
                        fprintf(stderr, mysql_error(conn));
                        abort_with_error("mysql_query()", 1);
                    }
                    bzero(query, 512);
                    xmlFree(id);
                    xmlFree(user_agent);

                }

                cur_node = cur_node->next;

            }

        } else {
            cur_node = cur_node->next;
        }

    }
//}

}

MYSQL *connect_DB(char *user, char *password, char *database) {

    char *server = "localhost";

    MYSQL *conn = mysql_init(NULL);

    /* Connect to database */
    if (!mysql_real_connect(conn, server, user, password, database, 3306, NULL, 0)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        //exit(EXIT_FAILURE);
    }

    return conn;
}


int main() {

    MYSQL *conn = connect_DB("root", "portento123", "sys");                                                             /* The password has to be taken from the configuration file */

    /*if (mysql_query(conn, "CREATE USER 'test_user'@'localhost' IDENTIFIED BY 'password';")) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        //mysql_close(conn);
        //exit(1);
    }*/

    if (mysql_query(conn, "CREATE DATABASE devicedb;")) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        //mysql_close(conn);
        //exit(1);
    }

    mysql_close(conn);

    conn = connect_DB("root", "portento123", "devicedb");

    if (mysql_query(conn, "DROP TABLE IF EXISTS device;")) {
        mysql_errno(conn);
        fprintf(stderr, mysql_error(conn));
        //abort_with_error("mysql_query()", 1);
    }

    if (mysql_query(conn,
                    "CREATE TABLE device (id VARCHAR(128), useragent VARCHAR(256), height INTEGER, width INT, png INT, jpg INT, tiff INT, colors INT);")) {
        mysql_errno(conn);
        fprintf(stderr, mysql_error(conn));
        abort_with_error("mysql_query()", 1);
    }

    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    /*parse the file and get the DOM */
    doc = xmlReadFile("./res/wurfl.xml", NULL, 0);

    /*Get the root element node */
    root_element = xmlDocGetRootElement(doc);

    //printf("PRIMA DI PRINT\n");

    populate_db(root_element, conn);

    /*free the document */
    xmlFreeDoc(doc);

    /*
     *Free the global variables that may
     *have been allocated by the parser.
     */
    xmlCleanupParser();

    return 0;

}