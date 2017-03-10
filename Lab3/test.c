/* CSCI 4061 Assignment 3
   Student ID: 
   Student Name: 
   test.c
*/

#include "mini_filesystem.h"
#include <stdio.h>

/* Test Helper Interface */
void write_into_filesystem(char* input_directory, char *log_filename);
void make_filesystem_summary(char* filename);
void read_images_from_filesystem_and_write_to_output_directory(char* output_directory);
void generate_html_file(char* filename);

/* Main function */
int main(int argc, char* argv[]){

        // Command line arguments: <executable-name> <input_dir> <output_dir> <log_filename>

        /*
                Fill in the codes
        */  	

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
  Read_File(3,0,8,buf);
  printf("BUF:|%s|\n",buf);
  print_memory();
  return 0;
}
