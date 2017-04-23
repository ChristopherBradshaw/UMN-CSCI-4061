/* Information
CSci 4061 Spring 2017 Assignment 5
Name1=Christopher Bradshaw
StudentID1=5300734
Commentary=Image server
*/

#include "client.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Prototypes */
int read_config(char *file);
char *read_val(const char *key, char *str, int len);
image_t *str_to_image_t(char *str);

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
    
  init_socket();

  read_catalog();

  /* If an image type is specified, enter passive mode */
  if(image_type) {
    printf("Passive\n");
  } else {
    printf("Interactive\n");
  }

  close(sockfd);
  return 0;
}

/* Initialize the connection */
void init_socket() {
  struct protoent *protoent;
  struct hostent *hostent;
  in_addr_t in_addr;
  struct sockaddr_in sockaddr_in;

  if((protoent = getprotobyname("tcp")) == NULL) {
    perror("getprotobyname"); 
    exit(EXIT_FAILURE);
  }

  if((sockfd = socket(AF_INET, SOCK_STREAM, protoent->p_proto)) == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  if((hostent = gethostbyname(server_ip)) == NULL) {
    fprintf(stderr,"error: gethostbyname(%s)\n",server_ip);
    exit(EXIT_FAILURE);
  }

  in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
  if(in_addr == (in_addr_t)-1) {
    fprintf(stderr,"error: inet_addr(%s)\n",*(hostent->h_addr_list));
    exit(EXIT_FAILURE);
  }

  sockaddr_in.sin_addr.s_addr = in_addr;
  sockaddr_in.sin_family = AF_INET;
  sockaddr_in.sin_port = htons(server_port);

  if(connect(sockfd,(struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) < 0) {
    perror("connect");
    exit(EXIT_FAILURE);
  }
  printf("Connected to %s on port %d.\n",server_ip,server_port);
}

/* Create a catalog entry from this line */
catalog_entry_t *build_catalog_entry(char *line) {
  char buf[256];
  catalog_entry_t *entry = malloc(sizeof(catalog_entry_t));
  char *res = line;
  char *tmp = line;
  int len;

  /* Get file name (first col) */
  tmp = strchr(tmp,',');
  len = tmp-res;
  strncpy(entry->filename,line,len);
  entry->filename[len] = '\0';
  ++tmp;

  /* Get file size (second col) */
  res = tmp;
  tmp = strchr(tmp,',');
  len = tmp-res;
  ++tmp;
  strncpy(buf,res,len);
  buf[len] = '\0';
  entry->filesize = atoi(buf);
  if(entry->filesize <= 0) {
    free(entry);
    return NULL;
  }

  /* Get checksum */
  strcpy(entry->checksum,tmp);

  return entry;
}

/* Parse through the catalog, */
void parse_catalog(char *str) {
  char copy[strlen(str)];
  strcpy(copy, str);
  char *tmp = strtok(copy,"\n");
  int add = 0;
  /* Go line by line and create an entry */
  while(tmp != NULL) {
    /* Skip first line (column names) */
    if(add) {
      catalog_entry_t *entry = build_catalog_entry(tmp);
      if(entry != NULL)
        catalog[catalog_idx++] = *entry; 
    }
    tmp = strtok(NULL, "\n");
    add = 1;
  }
}

/* Read/save catalog from the server */
int read_catalog() {
  /* Request the catalog file */
  write(sockfd,"0",1);

  /* File size is sent first, read it */
  uint32_t n;
  recv(sockfd,&n,sizeof(uint32_t),0); 

  /* Now read the actual file data */
  int catalog_size = ntohl(n);
  char file_buf[catalog_size+1];
  char buf[chunk_size];
  int total_read = 0;
  int rd;
  while(total_read < catalog_size && 
      (((rd = recv(sockfd,buf,5,0)) != 0) || errno == EAGAIN)) {
    strncpy(file_buf+total_read,buf,rd);
    total_read += rd;
  }
  file_buf[catalog_size] = '\0';
  parse_catalog(file_buf);
  return 0;
}

/* Read/save a file from the server */
int read_file(const char *fname) {
  return 0;
}

/* Read the specified value for a keyword/label. Ex: Port=8080 */
char *read_val(const char *key, char *str, int len) {
  char copy[len];
  strcpy(copy, str);
  char *tmp = strtok(copy,"\n");
  while(tmp != NULL) {
    char *res = strstr(tmp,key);
    if(res != NULL) {
      res = 1+strrchr(res,'=');
      while(*res == ' ')
        res++;
      char *ret = malloc(strlen(res));
      strcpy(ret,res);
      return ret;
    }
    tmp = strtok(NULL, "\n");
  }
  return NULL;
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

  /* Parse out each piece of info we need */
  char *ipstr = read_val("Server",buffer,rd);
  char *portstr = read_val("Port",buffer,rd);
  char *chunkstr = read_val("Chunk_Size",buffer,rd);
  char *imgtypestr = read_val("ImageType",buffer,rd);

  /* Make sure we have everything we need, then convert strs */
  if(!ipstr || !portstr || !chunkstr)
    return 1;
  
  strcpy(server_ip,ipstr);
  server_port = atoi(portstr);
  chunk_size = atoi(chunkstr);
  image_type = str_to_image_t(imgtypestr);

  if(imgtypestr)
    printf("Chunk size is %d. Image type is %s.\n", chunk_size, imgtypestr);
  else
    printf("Chunk size is %d. No image type specified.\n",chunk_size);

  /* Need to free these strs */
  free(ipstr);
  free(portstr);
  free(chunkstr);
  free(imgtypestr);
  return 0;
}

image_t *str_to_image_t(char *str) {
  // Change str to lower 
  char *iter = str;
  for( ; *iter; ++iter) *iter= tolower(*iter);
  image_t *t = malloc(sizeof(image_t));
  if(strcmp(str,"jpg") == 0) {
    *t = JPG; 
  } else if(strcmp(str,"png") == 0) {
    *t = PNG;
  } else if(strcmp(str,"gif") == 0) {
    *t = GIF;
  } else if(strcmp(str,"tiff") == 0) {
    *t = TIFF;
  } else {
    free(t);
    t = NULL;
  }
  return t;
}
