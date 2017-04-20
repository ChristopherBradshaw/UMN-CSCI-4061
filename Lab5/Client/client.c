#include "client.h"
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Read/extract client config */
int read_config(char *file);
/* Read value associated with key in config str */
char *read_val(const char *key, char *str, int len);

/* ./client <config file> */
int main(int argc, char **argv) {
  if(argc != 2) {
    fprintf(stderr,"Usage: %s <config file>\n",argv[0]);
    return 1;
  }
  
  if((read_config(argv[1])) != 0) {
    fprintf(stderr,"Fatal: Could not read config file\n");
    return 1;
  }
  
  return 0;
}

char *read_val(const char *key, char *str, int len) {
  return "";
}

/* Read/extract client config */
int read_config(char *file) {
  int BUF_LEN = 1024;
  char buffer[BUF_LEN];
  FILE *cfg;
  if((cfg = fopen(file,"r")) == NULL)
    return 1;

  int rd = fread(buffer,1,BUF_LEN,cfg);
  buffer[rd] = 0;
  char *ipstr = read_val("Server",buffer,rd);
  char *portstr = read_val("Port",buffer,rd);
  char *chunkstr = read_val("Chunk_Size",buffer,rd);
  char *imgtypestr = read_val("ImageType",buffer,rd);
  return 0;
}
