/* Information
CSci 4061 Spring 2017 Assignment 5
Name1=Christopher Bradshaw
StudentID1=5300734
Commentary=Image server
*/

#ifndef CLIENT_H
#define CLIENT_H

// Maximum number of catalog entries
#define MAX_CATALOG_N 256
#define OUTPUT_DIR "images"

typedef enum image_type {
  JPG, PNG, GIF, TIFF
} image_t;

typedef struct catalog_entry {
  char filename[128];
  char checksum[64];
  int filesize;
} catalog_entry_t;

/* Data */
static catalog_entry_t catalog[MAX_CATALOG_N]; // Catalog from server
static int catalog_idx = 0;
static char server_ip[128]; // Server IP address
static int server_port;  // Server port
static int chunk_size;  // Max number of bytes in a packet
static image_t *image_type; // Type of image to download (implies passive mode)
int sockfd; // File descriptor for connection to server

/* Functions */

/* Initialize the connection */
void init_socket(void);

/* Read/save catalog from the server */
int read_catalog(void);

/* Read/save a file from the server */
int download_file(int catalog_entry);

/* Print the catalog */
void dump_catalog(void);

/* Automatically download images of type image_type */
void do_passive(void);

/* Prompt the user for specific files */
void do_interactive(void);

/* Read/save a file from the server */
int read_file(const char *fname);

#endif
