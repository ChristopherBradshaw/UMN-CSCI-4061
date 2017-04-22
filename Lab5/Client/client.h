/* Information
CSci 4061 Spring 2017 Assignment 5
Name1=Christopher Bradshaw
StudentID1=5300734
Commentary=Image server
*/

#ifndef CLIENT_H
#define CLIENT_H

#define MAX_CATALOG_N 128
#define MAX_CATALOG_FLEN 128
typedef enum image_type {
  JPG, PNG, GIF, TIFF
} image_t;

/* Data */
static char catalog[MAX_CATALOG_N][MAX_CATALOG_FLEN+1]; // Catalog from server
static char server_ip[128]; // Server IP address
static int port;  // Server port
static int chunk_size;  // Max number of bytes in a packet
static image_t *image_type; // Type of image to download (implies passive mode)
int sockfd; // File descriptor for connection to server

/* Functions */

/* Initialize the connection */
void init_socket(void);

/* Read/save catalog from the server */
int read_catalog(void);

/* Read/save a file from the server */
int read_file(const char *fname);

#endif
