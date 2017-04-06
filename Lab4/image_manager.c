/* Information
CSci 4061 Spring 2017 Assignment 4
Name1=Christopher Bradshaw
StudentID1=5300734
Commentary=This program manages image files
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "image_manager.h"
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

/* Utility functions */
int read_dir(const char *dir_name);
const char *get_file_type(const char *f);

/* Thread subroutines */
void *do_v1(void *dir);
void *do_v2(void *dir);
void *do_v3(void *dir);

int main(int argc, char **argv) {
  /* Enforce command line arguments */
  if(argc != 4) {
    char *tmp;
    asprintf(&tmp,"Usage: %s [variant_options] [input_path] [output_dir]",
        argv[0]);
    write_output(tmp);
    return 1;
  }

  input_dir = argv[2];
  output_dir = argv[3];

  /* Make sure input directory exists and is readable */
  if(!read_dir(input_dir)) {
    write_output("Could not read input directory");
    return 1;
  }

  /* Create output directory if needed */
  if(!read_dir(output_dir)) {
    mkdir(output_dir,0700);
  }

  /* Read in the variant */
  variant_t *variant = NULL;
  if(strcmp(argv[1],"v1") == 0) {
    variant = malloc(sizeof(variant));
    *variant = V1;
  } else if(strcmp(argv[1],"v2") == 0) {
    variant = malloc(sizeof(variant));
    *variant = V2;
  } else if(strcmp(argv[1],"v3") == 0) {
    variant = malloc(sizeof(variant));
    *variant = V3;
  }

  /* Make sure variant is valid */
  if(variant == NULL) {
    write_output("Invalid variant option");
    return 1;
  }

  /* Open the log/output files, create if they doesn't exist */
  log_file = fopen(LOG_FILE,"ab+");
  remove(OUTPUT_FILE);
  output_file = fopen(OUTPUT_FILE,"ab+");

  /* Kick off the real work now */
  run_variant(*variant);

  /* All done, clean up everything */
  free(variant);
  fclose(log_file);
  fclose(output_file);

  return 0;
}

/* Traverses input directory (using a method specified by the variant).
 * Builds HTML file and handles logging. */
int run_variant(variant_t variant) { 
  /* This will be the first thread we run */
  pthread_t parent_thread;

  /* Determine a method based on the variant type */
  switch(variant) {
    case V1:
      write_output("Variant 1");
      pthread_create(&parent_thread,NULL,do_v1,(void *)input_dir);
      break;
    case V2:
      write_output("Variant 2");
      pthread_create(&parent_thread,NULL,do_v2,(void *)input_dir);
      break;
    case V3:
      write_output("Variant 3");
      pthread_create(&parent_thread,NULL,do_v3,(void *)input_dir);
      break;
    default:
      write_output("Invalid variant");
      return 1;
  }

  /* Join the thread and check status */
  void *status;
  int rc;
  if((rc = pthread_join(parent_thread,&status)) != 0) {
    /* Thread join hit an error */
    char *tmp;
    asprintf(&tmp,"Error; return code %d from pthread_join()",rc);
    write_output(tmp);
    return 1;
  }

  return 0;
}

/* Write to output file, must be thread safe */
void write_output(const char *str) {
  // TODO - make thread safe?
  fprintf(output_file,"%s\n",str);
}

/* Write to log file, must be thread safe */
void write_log(const char *str) {
  // TODO - make thread safe?
  fprintf(log_file,"%s\n",str);
}

/* Thread subroutines */
void *do_v1(void *input_dir) {
  DIR *dir;
  struct dirent *entry;
  const char *name = (char*)input_dir;
  char *tmp;

  // TODO -- assume MAX_SUBDIRS?
  pthread_t subdir_threads[MAX_SUBDIRS];
  int cur_subdir_thread = 0;

  // TODO remove this
  //asprintf(&tmp,"Found dir: %s",name);
  //write_output(tmp);

  if (!(dir = opendir(name))) {
    perror("Failed to open dir");
    return NULL;
  }
  if (!(entry = readdir(dir))) {
    perror("Failed to read dir");
    return NULL;
  }

  do {
    if (entry->d_type == DT_DIR) {
      if(cur_subdir_thread >= MAX_SUBDIRS) {
        asprintf(&tmp, "ERROR: Exceeded %d subdirectories in %s",MAX_SUBDIRS,name);
        write_output(tmp);
        break;
      }

      /* This is a directory */
      char *path; 
      asprintf(&path,"%s/%s",name,entry->d_name);
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
          continue;
      pthread_create(&subdir_threads[cur_subdir_thread++],NULL,do_v1,(void *)path);
    } else {
      /* This is a file, write it to the HTML file */
      char *tmp;
      // TODO remove this
      //asprintf(&tmp,"%s/%s",name,entry->d_name);
      //write_output(tmp);
      // If the file is an image type, add it to HTML file 
      // (make macros for testing each type) 
    }
  } while ((entry = readdir(dir)));
  closedir(dir);

  /* Join all subdirectory threads before exiting */
  int i;
  for(i = 0; i < cur_subdir_thread; i++) {
    void *status;
    pthread_join(subdir_threads[i],&status);
  }

  pthread_exit((void*) input_dir);
}

void *do_v2(void *input_dir) {

  pthread_exit((void*) input_dir);
}

void *do_v3(void *input_dir) {

  pthread_exit((void*) input_dir);
}

/* Initialize the HTML file */
void init_html() {

}

/* Write entry to HTML file */
void write_html(const file_struct_t *file) {

}

/* Complete and close the HTML file */
void finish_html() {

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

/* Return the file extension (ex: a.jpg -> jpg) */
const char* get_file_type(const char *f_name)
{
  char *tmp = strrchr(f_name,'.');

  /* Doesn't have an extension */
  if(tmp == NULL)
    return NULL;

  return tmp+1;
}

