/* Information
CSci 4061 Spring 2017 Assignment 5
Name1=Christopher Bradshaw
StudentID1=5300734
Commentary=Image server
*/

#ifndef SERVER_H
#define SERVER_H
#define MAX_WAITING 10 // Maximum number of waiting connections
#define CATALOG_FILE "catalog.csv"

typedef enum image_type {
  JPG, PNG, GIF, TIFF
} image_t;

/* Data */
static int port;
static char server_dir[128];
int sockfd;

/* Functions */

/* Search directories and find path for this file */
char *get_abs_filepath(char *fname);

/* Initialize the connection */
void init_socket(void);

#endif
