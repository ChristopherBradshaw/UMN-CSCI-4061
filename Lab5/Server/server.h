/* Information
CSci 4061 Spring 2017 Assignment 5
Name1=Christopher Bradshaw
StudentID1=5300734
Commentary=Image server
*/

#ifndef SERVER_H
#define SERVER_H
#define MAX_WAITING 10 // Maximum number of waiting connections
#define MAX_NUM_FILES 256
#define MAX_FILE_LEN 1024
#define CATALOG_FILE "catalog.csv"

typedef enum image_type {
  JPG, PNG, GIF, TIFF
} image_t;

/* Data */
static int port;
static char server_dir[128];
static int sockfd;
static char file_list[MAX_NUM_FILES][MAX_FILE_LEN];

/* Functions */

/* Search directories and find path for this file */
char *get_abs_filepath(char *fname);

/* Searches for images, builds paths for them and writes the catalog file */
void build_catalog();

/* Initialize the connection */
void init_socket(void);

#endif
