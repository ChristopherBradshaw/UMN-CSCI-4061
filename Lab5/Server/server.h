#ifndef CLIENT_H
#define CLIENT_H

typedef enum image_type {
  JPG, PNG, GIF, TIFF
} image_t;

static int port;
static char server_dir[128];
int sockfd; //Socket file descriptor

#endif
