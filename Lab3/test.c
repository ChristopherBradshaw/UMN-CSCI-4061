/* CSCI 4061 Assignment 3
   Student ID: 
   Student Name: 
   test.c
*/

#include "mini_filesystem.h"

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


  Initialize_Filesystem("asdf");
  Create_File("potato",5,5,4);
  Open_File("potato");
  printf("Wrote: %d\n",Write_File(0,0,"12345"));
  print_memory();
  return 0;
}
