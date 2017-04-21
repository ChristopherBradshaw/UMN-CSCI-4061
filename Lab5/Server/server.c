#include "server.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <ctype.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int read_config(char *file);
char *read_val(const char *key, char *str, int len);
image_t *str_to_image_t(char *str);
void init_socket();

/* ./server <config file> */
int main(int argc, char **argv) {
  if(argc != 2) {
    fprintf(stderr,"Usage: %s <config file>\n",argv[0]);
    return 1;
  }
  
  if((read_config(argv[1])) != 0) {
    fprintf(stderr,"Fatal: Could not read config file\n");
    return 1;
  }
    
  //init_socket();
  return 0;
}

/*
void init_socket() {
  struct protoent *protoent;
  struct hostent *hostent;
  in_addr_t in_addr;
  in_addr_t server_addr;
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
  sockaddr_in.sin_port = htons(port);

  if(connect(sockfd, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) < 0) {
    perror("connect");
    exit(EXIT_FAILURE);
  }

  write(sockfd,"potato",16);
}
*/

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
  char *portstr = read_val("Port",buffer,rd);
  char *dirstr = read_val("Dir",buffer,rd);

  /* Conver strs to appropriate format if needed */
  strcpy(server_dir,dirstr);
  port = atoi(portstr);

  printf("Dir: %s, Port: %s\n",dirstr,portstr);
  /* Need to free these strs */
  free(portstr);
  free(dirstr);
  return 0;
}
