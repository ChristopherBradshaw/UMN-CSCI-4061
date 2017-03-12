/* CSCI 4061 Assignment 3
   Student ID: 
   Student Name: 
   test.c
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "mini_filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* Test Helper Interface */
void write_into_filesystem(char* input_directory, char *log_filename);
void make_filesystem_summary(char* filename);
void read_images_from_filesystem_and_write_to_output_directory(char* output_directory);
void generate_html_file(char* dir, char* html_filename);

void some_tests(void);
/* Main function */
int main(int argc, char* argv[])
{

  /* Check command line arguments */
  if(argc != 4)
  {
    fprintf(stderr,"USAGE: %s <input_dir> <output_dir> <log_filename>\n",argv[0]);
    return -1;
  }
  
  SET_LOG_LEVEL(INFO);
  write_into_filesystem(argv[1],argv[3]);
  make_filesystem_summary("summary");
  read_images_from_filesystem_and_write_to_output_directory(argv[2]);
  generate_html_file(argv[2],"filesystem_content.html");
  return 0;
}

/* Return the file extension (ex: a.jpg -> jpg) */
char* get_file_type(char *f_name)
{
	char *tmp = strrchr(f_name,'.');
		
	/* Doesn't have an extension */
	if(tmp == NULL)
		return NULL;
				
	return tmp+1; 
}

/* Returns a new file name string without the extension (ex: a.jpg -> a) */
char *get_file_name_no_ext(char* mystr)
{
		char *retstr;
		char *lastdot;
		if (mystr == NULL)
				 return NULL;
		if ((retstr = malloc (strlen (mystr) + 1)) == NULL)
				return NULL;
		strcpy (retstr, mystr);
		lastdot = strrchr (retstr, '.');
		if (lastdot != NULL)
				*lastdot = '\0';
		return retstr;
}
 
/* Copy the specified file to the virtual filesystem */
int write_single_file(char *filename)
{
  char *buffer = NULL;
	long length;
	FILE *f = fopen (filename, "rb");
	if(f)
	{
    /* Add the file into a buffer, find the length */
		fseek (f, 0, SEEK_END);
		length = ftell (f);
		fseek (f, 0, SEEK_SET);
		buffer = malloc (length+1);
		if (buffer)
		{
			fread (buffer, 1, length, f);
		}
		fclose (f);
	}
  else
  {
    fprintf(stderr,"Failed to read: %s\n",filename);
    return -1;
  }

	if (buffer)
	{
    /* Only write the basename to the directory structure (not the full path),
     * then write the buffer contents to our virtual filesystem */
    char *f_basename = basename(filename);
    int inode_idx;
    if((inode_idx = Create_File(f_basename,5,5,length)) == -1)
    {
      fprintf(stderr,"Failed to create %s. Does it already exist?\n",f_basename);
      return -1;
    }

    Write_File(inode_idx,0,length,buffer);
    free(buffer);
	}
  else
  {
    fprintf(stderr,"Failed to create buffer for: %s\n",filename);
    return -1;
  }

  return 1;
}

/* Recursively the input directory, write all files to our filesystem */
void traverse_input_dir(char *name, int level)
{
  DIR *dir;
  struct dirent *entry;

  if (!(dir = opendir(name)))
      return;
  if (!(entry = readdir(dir)))
      return;

  do {
      if (entry->d_type == DT_DIR) {
        /* This is a directory */
        char path[1024];
        int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
        path[len] = 0;
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        traverse_input_dir(path, level + 1);
      }
      else
      {
        /* This is a file, write it to our filesystem */
        char *tmp;
        asprintf(&tmp,"%s/%s",name,entry->d_name);
        write_single_file(tmp);
      }
  } while ((entry = readdir(dir)));
  closedir(dir);
}

void reset_file(char *filename)
{
  /* Remove the file if it already exists */
  if(access(filename,F_OK) != -1) 
    remove(filename);

  /* Now create it */
  int fd;
  if((fd = open(filename, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO)) == -1)
  {
    fprintf(stderr,"Failed to create file: %s\n",filename);
    return; 
  }

  close(fd);
}

/* Set up the log file, recursively traverse the input directory, adding
 * every file we find into our virtual filesystem */
void write_into_filesystem(char *input_directory, char *log_filename)
{
  reset_file(log_filename); 
  Initialize_Filesystem(log_filename);
  traverse_input_dir(input_directory,0);
}

void make_filesystem_summary(char* filename)
{
  reset_file(filename);
  FILE *out = fopen(filename,"w");
	int i;
  for(i = 0; i < directory_size; i++)
  {
    Directory dtmp = Directory_Structure[i];
		Inode itmp = Inode_List[i];

    fprintf(out,"[file:%s, extension: %s, inode:%d, size:%d]\n",dtmp.Filename,
        get_file_type(dtmp.Filename),itmp.Inode_Number,itmp.File_Size);
  }
  fclose(out);
}

/* Read all JPG images from the filesystem and write to the output directory */
void read_images_from_filesystem_and_write_to_output_directory(char* output_directory)
{
  /* Make sure output directory exists*/
	DIR* dir = opendir(output_directory);
	if (dir)
	{
    /* Directory already exists. */
    closedir(dir);
	}
	else if (ENOENT == errno)
	{
    /* Directory doesn't exist, make it*/
    mkdir(output_directory,0700);
	}

  int i;
  for(i = 0; i < directory_size; i++)
  {
    Directory dir_entry = Directory_Structure[i];

    if(strcmp(get_file_type(dir_entry.Filename),"jpg") != 0)
      continue;

    /* Get the INode number for this file (make sure it exists) */
    int inode_idx;
    if((inode_idx = Open_File(dir_entry.Filename)) == -1)
    {
      fprintf(stderr,"Could not open %s\n",dir_entry.Filename);
      continue;
    }

    /* Get filesize so we know how many bytes to read */
    int filesize = Get_Filesize(inode_idx);
    if(filesize == -1)
    {
      fprintf(stderr,"Failed to get filesize for %s\n",dir_entry.Filename);
      continue;
    }

    char *buffer = malloc(filesize);
    int n_read = Read_File(inode_idx,0,filesize,buffer);

    /* Determine the filepath, clear it if it already exists */
    char *dst; 
    asprintf(&dst,"%s/%s",output_directory,dir_entry.Filename);
    if(access(dst,F_OK) != -1)
      remove(dst);

    /* Write the contents of the buffer into the file */
    FILE *f = fopen(dst,"ab+");
    int j;
    for(j = 0; j < n_read; j++)
    {
      fprintf(f,"%c",buffer[j]);
    }

    fclose(f);
    Close_File(inode_idx);
  }
}

/* Restarts wait if interrupted by a signal (Robbins pg. 72) */
pid_t r_wait(int *stat_loc)
{
	int retval;
	while(((retval = wait(stat_loc)) == -1) && (errno == EINTR)) ;
	return retval;
}

void generate_html_file(char* dir, char* html_filename)
{
	DIR *input_dp;
	struct dirent *input_ep;
	input_dp = opendir(dir);

  /* Remove the HTML file if it already exists */
  if(access(html_filename,F_OK) != -1)
    remove(html_filename);  

  FILE *html = fopen(html_filename,"ab+");
	if(input_dp != NULL)
	{ 
    fprintf(html,"<html>\n\t<head>\n\t\t<title>Images</title>\n\t</head>\n\t<body>");
    while((input_ep = readdir(input_dp)))
		{ 
			/* Skip over files like . and .. AND skip over thumbnail images */
			if(input_ep->d_type != DT_REG || strstr(input_ep->d_name,"_thumb"))
				continue;
			
      /* Make sure we're dealing with a jpg */
			char *type = (char *) get_file_type(input_ep->d_name);
      if(strcmp(type,"jpg") != 0)
        continue;
		  
      char *src;
      asprintf(&src,"%s/%s",dir,input_ep->d_name);
      char *dst;
      asprintf(&dst,"%s/%s_thumb.jpg",dir,get_file_name_no_ext(input_ep->d_name));

      if(fork() != 0)
      {
        /* We're the parent, build on the HTML */
        fprintf(html,"\n\t\t\t<a href=\"%s\">\n",src);
        fprintf(html,"\t\t\t<img src=\"%s\"/></a>\n",dst);
      }
      else
      {
        /* We're the child, convert this file */
        execlp("convert","convert","-resize","200x200",src,dst,NULL);
      }

		}
    fprintf(html,"\t</body>\n</html>");
	}
	else
	{ 
		perror("Could not read input directory");
	}

  /* Wait for all children to finish conversion */
  while(r_wait(NULL) > 0) ;
  fclose(html);
}
