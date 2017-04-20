#ifndef CLIENT_H
#define CLIENT_H

typedef enum image_type {
  JPG, PNG, GIF, TIFF
} image_t;

// DATA
static char server_ip[128];
static int port;
static int chunk_size;
static image_t *image_type;

#endif