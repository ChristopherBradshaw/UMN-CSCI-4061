/* CSCI 4061 Assignment 3
   Student ID: 
   Student Name: 
   test.c
*/

#include "mini_filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

/* Test Helper Interface */
void write_into_filesystem(char* input_directory, char *log_filename);
void make_filesystem_summary(char* filename);
void read_images_from_filesystem_and_write_to_output_directory(char* output_directory);
void generate_html_file(char* filename);

void some_tests(void);
/* Main function */
int main(int argc, char* argv[])
{

        // Command line arguments: <executable-name> <input_dir> <output_dir> <log_filename>
  if(argc != 4)
  {
    fprintf(stderr,"USAGE: %s <input_dir> <output_dir> <log_filename>\n",argv[0]);
    return -1;
  }
  write_into_filesystem(argv[1],argv[3]);
  return 0;
}

int write_single_file(char *filename)
{
  char * buffer = 0;
	long length;
	FILE * f = fopen (filename, "rb");

	if (f)
	{
		fseek (f, 0, SEEK_END);
		length = ftell (f);
		fseek (f, 0, SEEK_SET);
		buffer = malloc (length);
		if (buffer)
		{
			fread (buffer, 1, length, f);
		}
		fclose (f);
	}

	if (buffer)
	{
    //Handle
	}

  Create_File(filename,5,5,length);
  Write_File(0,0,buffer);
  char buf[512];
  printf("Read %d bytes\n",Read_File(0,0,length,buf));
  printf("|%s|\n",buf);
  free(buffer);
  return 1;
}

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
          char path[1024];
          int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
          path[len] = 0;
          if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
              continue;
          printf("%*s[%s]\n", level*2, "", entry->d_name);
          traverse_input_dir(path, level + 1);
      }
      else
          printf("%*s- %s\n", level*2, "", entry->d_name);
  } while (entry = readdir(dir));
  closedir(dir);
}

void write_into_filesystem(char *input_directory, char *log_filename)
{
  Initialize_Filesystem(log_filename);
  traverse_input_dir(input_directory,0);
  write_single_file("a.txt");
  // Scan input directory
  // If file exists in directory structure
}

void make_filesystem_summary(char* filename)
{

}

void read_images_from_filesystem_and_write_to_output_directory(char* output_directory)
{

}

void generate_html_file(char* filename)
{

}

void some_tests()
{
  SET_LOG_LEVEL(INFO);

  Initialize_Filesystem("asdf");

  Create_File("potato",5,5,32);
  Create_File("test",5,5,6);
  Create_File("test2",5,5,6);
  Create_File("test3",5,5,26);
  Create_File("test4",5,5,8);
  
  Write_File(0,0,"01234567890123456789testpassluck");
  Write_File(1,0,"abcd&&");
  Write_File(2,0,"test^^");
  Write_File(3,0,"abcdefghijklmnopqrstuvwxyz");
  Write_File(4,0,"thisxxyy");

  char buf[256];
  int i;
  for(i = 0; i < 5; i++)
  {
    if(Read_File(i,0,8,buf))
      printf("BUF:|%s|\n",buf);
  }
}
