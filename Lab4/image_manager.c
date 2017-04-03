/* Information
CSci 4061 Spring 2017 Assignment 4
Name1=Christopher Bradshaw
StudentID1=5300734
Commentary=This program manages image files
*/

#include "image_manager.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>

int read_dir(const char *dir_name);

int main(int argc, char **argv) {
  /* Enforce command line arguments */
  if(argc != 4) {
    fprintf(stderr,"Usage: %s [variant_options] [input_path] [output_dir]\n",
        argv[0]);
    return 1;
  }

  /* Make sure variant is correct */
  if(strcmp(argv[1],"v1") && strcmp(argv[1],"v2") && strcmp(argv[1],"v3")) {
    fprintf(stderr,"Invalid variant option\n");
    return 1;
  }

  /* Make sure input directory exists and is readable */
  if(!read_dir(argv[2])) {
    fprintf(stderr,"Could not read input directory\n");
    return 1;
  }

  return 0;
}

/* Attempt to read the specified directory, return zero if failure, non-zero
 * if success*/
int read_dir(const char *dir_name) {
  DIR* dir = opendir(dir_name);
  if(dir) {
    closedir(dir);
    return 1;
  }
  return 0;
}
