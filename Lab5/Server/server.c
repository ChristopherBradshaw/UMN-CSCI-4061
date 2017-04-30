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
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <ftw.h>
#include <dirent.h>
#include "../md5/md5sum.h"

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
    
  DIR* check;
  if((check = opendir(server_dir)) == NULL) {
    fprintf(stderr,"Fatal: Could not read input directory: %s\n",server_dir);
    return 1;
  }
  closedir(check);

  build_catalog();
  init_socket();
  handle_cons();
  return 0;
}

/* Return the file extension (ex: a.jpg -> jpg) */
char* get_file_type(const char *f_name)
{
  char *tmp = strrchr(f_name,'.');

  /* Doesn't have an extension */
  if(tmp == NULL)
    return NULL;

  return tmp+1;
}

int visit(const char *name, const struct stat *status, int type) {
	if(type != FTW_F)
  	return 0;
  status = NULL;
  char *ftype = get_file_type(name);
  char *t;
  for(t = ftype; *t; ++t) *t = tolower(*t);   // Convert type to lower case
  if(strcmp(ftype,"jpg") == 0 || strcmp(ftype,"png") == 0 || strcmp(ftype,"gif") == 0 ||
      strcmp(ftype,"tiff") == 0) {
    // This is an image
    strcpy(file_list[file_list_idx++],name);
  }
  return 0;
}

void build_catalog() {
  remove(CATALOG_FILE);
  FILE *fp;
  if((fp = fopen(CATALOG_FILE,"ab+")) == NULL) {
    perror("failed to open catalog file");
    return;
  }
  ftw(server_dir,visit,20);
  fprintf(fp,"filename, size, checksum\n");
  int i;
  // Visit every file we added in the ftw call
  for(i = 0; i < file_list_idx; i++) {
    // For each file, print a 3-tuple to the file containing
    // basefilename, size, md5
    char *fname = file_list[i];
    struct stat st;
    stat(fname,&st);
    fprintf(fp,"%s, %lu, ",basename(fname),st.st_size);
    unsigned char sum[MD5_DIGEST_LENGTH];
    md5sum(fname,sum);
    int j;
    for(j = 0; j < MD5_DIGEST_LENGTH; j++) {
      fprintf(fp,"%02x",sum[j]);
    }
    fprintf(fp,"\n");
  }
  fclose(fp);
  printf("Built catalog\n");
}

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* Load the specified file into the buffer and set the size */
void load_file(const char *fname, char **buf, int *size) {
	FILE *fp;
	long lSize;
	char *buffer;

	fp = fopen (fname,"rb");
	if(!fp) {
    perror(fname);
    *buf = NULL;
    *size = 0;
    return;
  }

	fseek(fp,0L,SEEK_END);
	lSize = ftell(fp);
	rewind(fp);

	/* allocate memory for entire content */
	buffer = calloc(1,lSize+1);
	if(!buffer) {
    fclose(fp);
    fputs("failed file alloc",stderr);
    exit(1);
  }

	/* copy the file into the buffer */
	if(fread(buffer,lSize,1,fp) != 1) {
		fclose(fp);
    free(buffer);
    fputs("failed file read",stderr);
    exit(1);
  }

	/* do your work here, buffer is a string contains the whole text */
  *buf = buffer;
  *size = lSize;
	fclose(fp);
}

/* Search directories and find path for this file */
char *get_abs_filepath(char *fname) {
  int i;
  for(i = 0; i < file_list_idx; i++) {
    char *tmp = file_list[i];
    if(strcmp(basename(tmp),fname) == 0) {
      return tmp;
    }
  }
  return NULL;
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
      while((n_read = recv(new_fd,tmp_buf,1,0)) != 0 
          || errno == EAGAIN) {
        if(tmp_buf[0] == '0') {
          printf("Catalog request: %s\n",s);
          char *buf = NULL;
          int file_size = 0;
          uint32_t un;
          /* Load file into a buffer, first send size, then send contents */
          load_file(CATALOG_FILE,&buf,&file_size);
          un = htonl(file_size);
          send(new_fd,&un,sizeof(uint32_t),0);
          send(new_fd,buf,file_size,0);
        } else if(tmp_buf[0] == '1') {
					/* File name length is sent first, read it */
					uint32_t n;
					recv(new_fd,&n,sizeof(uint32_t),0);
          int fnamelen = ntohl(n);
          char fname[fnamelen+1];
          recv(new_fd,fname,fnamelen,0);
          fname[fnamelen] = '\0';
          printf("File request [%s]: %s\n",fname,s);
          char *abs_filepath = get_abs_filepath(fname);
          if(abs_filepath == NULL) {
            close(new_fd);
            break;
          }
          FILE *fp;
          if((fp = fopen(abs_filepath,"rb")) == NULL) {
            printf("Could not open file %s\n",abs_filepath);
          }

          /* Get filesize */
          struct stat st;
          fstat(fileno(fp),&st);
          int fsize = st.st_size;

          /* Use sendfile to do all the work */
          long offset = 0;
          sendfile(new_fd,fileno(fp),&offset,fsize);
          fclose(fp);
        }
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
