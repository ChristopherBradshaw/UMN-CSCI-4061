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
#include <sys/types.h>
#include <fcntl.h>

/* Utility functions */
int read_dir(const char *dir_name);
char *get_file_type(const char *f);
#define IS_JPG(f) (strcmp("jpg",get_file_type(f)) == 0)
#define IS_PNG(f) (strcmp("png",get_file_type(f)) == 0)
#define IS_BMP(f) (strcmp("bmp",get_file_type(f)) == 0)
#define IS_GIF(f) (strcmp("gif",get_file_type(f)) == 0)
#define IS_IMAGE(f) (IS_JPG(f) || IS_PNG(f) || IS_BMP(f) || IS_GIF(f))

/* Thread subroutines */
void *do_v1(void *dir);
void *do_v2(void *dir);
void *do_v2_help(void *v2struct);
void *do_v3(void *dir);
file_struct_t *build_file_struct(const char *file); 

int main(int argc, char **argv) {
  /* Enforce command line arguments */
  if(argc != 4) {
    fprintf(stderr,"Usage: %s [variant_options] [input_path] [output_dir]\n",
        argv[0]);
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
  char *tmp;
  asprintf(&tmp,"%s/%s",output_dir,LOG_FILE);
  log_file = fopen(tmp,"ab+");
  remove(OUTPUT_FILE);
  asprintf(&tmp,"%s/%s",output_dir,OUTPUT_FILE);
  output_file = fopen(tmp,"ab+");

  /* Init mutexes */
  pthread_mutex_init(&log_mutex, NULL);
  pthread_mutex_init(&output_mutex, NULL);
  pthread_mutex_init(&html_mutex, NULL);

  /* Kick off the real work now */
  init_html();
  run_variant(*variant);

  /* All done, clean up everything */
  free(variant);
  fclose(log_file);
  fclose(output_file);
  finish_html();
  pthread_mutex_destroy(&log_mutex);
  pthread_mutex_destroy(&output_mutex);
  pthread_mutex_destroy(&html_mutex);

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
  pthread_mutex_lock(&output_mutex);
  fprintf(output_file,"%s\n",str);
  pthread_mutex_unlock(&output_mutex);
}

/* Write to log file, must be thread safe */
void write_log(const char *str) {
  pthread_mutex_lock(&log_mutex);
  fprintf(log_file,"%s\n",str);
  pthread_mutex_unlock(&log_mutex);
}

/* Thread subroutines */
void *do_v1(void *input_dir) {
  DIR *dir;
  struct dirent *entry;
  const char *name = (char*)input_dir;
  char *tmp;

  pthread_t subdir_threads[MAX_SUBDIRS];
  int cur_subdir_thread = 0;

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
      /* This is a file, check if it's an image */
      char *tmp;
      asprintf(&tmp,"%s/%s",name,entry->d_name);
      if(IS_IMAGE(entry->d_name)) {
        /* It's an image, write its information to the HTML file */
        file_struct_t *tuple = build_file_struct(tmp);
        write_html(tuple);
        free(tuple);
      }
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

void *do_v2_help(void *v2struct) {
  v2struct_data_t *data = (v2struct_data_t *) v2struct;
  DIR *dir;
  struct dirent *entry;
  const char *name = data->dir;

  // We might need to spawn more threads if we're the "directory" thread
  pthread_t *subdir_threads = NULL;
  int cur_subdir_thread = 0;
  if(data->type == DIRECTORY) {
    subdir_threads = calloc(MAX_SUBDIRS,sizeof(pthread_t));
  }

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
      // This is a directory, check if we're the "directory" thread
      // (which handles subdirectories)
      if(data->type != DIRECTORY || strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
          continue;

      char *tmp;
      if(cur_subdir_thread >= MAX_SUBDIRS) {
        asprintf(&tmp, "ERROR: Exceeded %d subdirectories in %s",MAX_SUBDIRS,name);
        write_output(tmp);
        break;
      }
      char *path; 
      asprintf(&path,"%s/%s",name,entry->d_name);
      pthread_create(&subdir_threads[cur_subdir_thread++],NULL,do_v2,(void *)path);
    } else {
      // This is a file, check if we handle it
      char *tmp;
      asprintf(&tmp,"%s/%s",name,entry->d_name);
      if(!IS_IMAGE(entry->d_name))
        continue;

      int shouldHandle = 0;
      switch(data->type) {
        case DIRECTORY:
          // Shouldn't happen
          break;
        case JPG:
          if(IS_JPG(entry->d_name))
            shouldHandle = 1;
          break;
        case PNG:
          if(IS_PNG(entry->d_name))
            shouldHandle = 1;
          break;
        case BMP:
          if(IS_BMP(entry->d_name))
            shouldHandle = 1;
          break;
        case GIF:
          if(IS_GIF(entry->d_name))
            shouldHandle = 1;
          break;
      }

      // If this file type "belongs" to us
      if(shouldHandle) {
        file_struct_t *tuple = build_file_struct(tmp);
        write_html(tuple);
        free(tuple);
      }
    }
  } while ((entry = readdir(dir)));
  closedir(dir);

  /* Join all subdirectory threads before exiting */
  int i;
  for(i = 0; i < cur_subdir_thread; i++) {
    void *status;
    pthread_join(subdir_threads[i],&status);
  }

  if(subdir_threads != NULL) {
    free(subdir_threads);
    subdir_threads = NULL;
  }

  pthread_exit((void*) input_dir);
}

// Dispatch 5 threads for this directory: one for subdirs 
// and one for each of the 4 image types
void *do_v2(void *input_dir) {
  char *input_dir_s = (char *) input_dir;
  pthread_t dir_th, jpg_th, png_th, bmp_th, gif_th;
  v2struct_data_t dir_data, jpg_data, png_data, bmp_data, gif_data;

  dir_data.type = DIRECTORY;
  jpg_data.type = JPG;
  png_data.type = PNG;
  bmp_data.type = BMP;
  gif_data.type = GIF;

  dir_data.dir = input_dir_s;
  jpg_data.dir = input_dir_s;
  png_data.dir = input_dir_s;
  bmp_data.dir = input_dir_s;
  gif_data.dir = input_dir_s;

  pthread_create(&dir_th,NULL,do_v2_help,(void *) &dir_data);
  pthread_create(&jpg_th,NULL,do_v2_help,(void *) &jpg_data);
  pthread_create(&png_th,NULL,do_v2_help,(void *) &png_data);
  pthread_create(&bmp_th,NULL,do_v2_help,(void *) &bmp_data);
  pthread_create(&gif_th,NULL,do_v2_help,(void *) &gif_data);

  pthread_join(dir_th, NULL);
  pthread_join(jpg_th, NULL);
  pthread_join(png_th, NULL);
  pthread_join(bmp_th, NULL);
  pthread_join(gif_th, NULL);

  pthread_exit((void*) input_dir);
}

void *do_v3(void *input_dir) {

  pthread_exit((void*) input_dir);
}

/* Return a new file struct with the information of this file */
file_struct_t *build_file_struct(const char *file) {
  int fd;
  if((fd = open(file,O_RDONLY)) < 0) {
    char *tmp;
    asprintf(&tmp,"Error while opening %s",file);
    write_output(tmp);
    return NULL;
  }

  struct stat file_stat;
  int ret;
  if((ret = fstat(fd, &file_stat)) < 0) {
    char *tmp;
    asprintf(&tmp,"Error while stat %s",file);
    write_output(tmp);
    return NULL;
  }

  file_struct_t *new_tuple = malloc(sizeof(file_struct_t));
  strcpy(new_tuple->FileName, file);
  new_tuple->FileId = file_stat.st_ino;
  new_tuple->FileType = get_file_type(file);
  new_tuple->Size = file_stat.st_size;
  new_tuple->TimeOfModification = file_stat.st_mtim;
  new_tuple->ThreadId = pthread_self();
  return new_tuple;
}

/* Initialize the HTML file */
void init_html() {
  char *tmp;
  asprintf(&tmp,"%s/%s",output_dir,HTML_FILE);
  remove(tmp);
  html_file = fopen(tmp,"ab+");
  fprintf(html_file,"<html><head><title>Image Manager</title></head><body>\n");
}

/* Write entry to HTML file -- THREAD SAFE */
void write_html(const file_struct_t *file) {
  if(file == NULL)
    return;

  pthread_mutex_lock(&html_mutex);
  fprintf(html_file,"<a href=../%s><img src=../%s width=100 height = 100></img></a>\
      <p align=\"left\">%d, %s, %s, %d, time, %ld</p>",file->FileName,file->FileName,
      file->FileId, file->FileName, file->FileType, file->Size, file->ThreadId);  
  pthread_mutex_unlock(&html_mutex);
}

/* Complete and close the HTML file */
void finish_html() {
  fclose(html_file);
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
char* get_file_type(const char *f_name)
{
  char *tmp = strrchr(f_name,'.');

  /* Doesn't have an extension */
  if(tmp == NULL)
    return NULL;

  return tmp+1;
}

