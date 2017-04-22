/* Information
CSci 4061 Spring 2017 Assignment 5
Name1=Christopher Bradshaw
StudentID1=5300734
Commentary=Image server
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "server.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>

int read_config(char *file);
char *read_val(const char *key, char *str, int len);
image_t *str_to_image_t(char *str);
void handle_cons();

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
    
  init_socket();
  handle_cons();
  return 0;
}

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* Listen for connections */
void handle_cons() {
  int new_fd;
	socklen_t sin_size;
  struct sockaddr_storage their_addr;
  char s[INET6_ADDRSTRLEN];

	while(1) {
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
      perror("accept");
      continue;
		}

		inet_ntop(their_addr.ss_family,
      get_in_addr((struct sockaddr *)&their_addr),
      s, sizeof s);
		printf("New connection: %s\n", s);

    // Create a new process for each connection
		if (fork() == 0) {
      char tmp_buf[10];
      int n_read = 0;
      while((n_read = recv(new_fd,tmp_buf,10,0)) != 0 
          || errno == EAGAIN) {
        printf("Read (%d): %s\n",n_read,tmp_buf);
      }
      printf("Terminated connection: %s\n",s);
      close(new_fd);
		} else {
		  close(new_fd);
    }
	}
}

void sigchld_handler(int s) {
  s = 0;
  int saved_errno = errno;
  while(waitpid(-1, NULL, WNOHANG) > 0);
  errno = saved_errno;
}

void init_socket() {
  struct addrinfo hints, *servinfo, *p;
  struct sigaction sa;
  int yes=1;
  int rv;

  /* Set connection hints */
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  char *port_str;
  asprintf(&port_str,"%d",port);
  if ((rv = getaddrinfo(NULL, port_str, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return;
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
        p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
          sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo); // all done with this structure

  if (p == NULL)  {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  if (listen(sockfd, MAX_WAITING) == -1) {
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  printf("Listening on port %d\n",port);
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
  char *portstr = read_val("Port",buffer,rd);
  char *dirstr = read_val("Dir",buffer,rd);

  /* Conver strs to appropriate format if needed */
  strcpy(server_dir,dirstr);
  port = atoi(portstr);

  /* Need to free these strs */
  free(portstr);
  free(dirstr);
  return 0;
}
