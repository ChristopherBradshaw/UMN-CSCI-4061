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
#include <signal.h>

/* Utility functions */
int read_dir(const char *dir_name);
char *get_file_type(const char *f);
file_struct_t *build_file_struct(const char *file); 
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
void *do_v3_img(void *file);
void *do_log(void *arg);

int main(int argc, char **argv) {
  time(&start_time);

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
    fprintf(stderr,"Invalid variant option\n");
    return 1;
  }

  /* Open the log/output files, create if they doesn't exist */
  char *tmp;
  asprintf(&tmp,"%s/%s",output_dir,LOG_FILE);
  log_file = fopen(tmp,"ab+");
  remove(OUTPUT_FILE);
  asprintf(&tmp,"%s/%s",output_dir,OUTPUT_FILE);
  remove(tmp);
  output_file = fopen(tmp,"ab+");
  num_dirs = num_jpg = num_bmp = num_png = num_gif = num_threads = 0;
  asprintf(&tmp,"\nVariant %s\n", argv[1]);
  write_log(tmp);
  write_log("----------------------------------");
  pthread_create(&log_thread,NULL,do_log,NULL);
  inc_threads();

  /* Init mutexes */
  pthread_mutex_init(&log_mutex, NULL);
  pthread_mutex_init(&output_mutex, NULL);
  pthread_mutex_init(&html_mutex, NULL);
  pthread_mutex_init(&num_dirs_mutex, NULL);
  pthread_mutex_init(&num_jpg_mutex, NULL);
  pthread_mutex_init(&num_bmp_mutex, NULL);
  pthread_mutex_init(&num_png_mutex, NULL);
  pthread_mutex_init(&num_gif_mutex, NULL);
  pthread_mutex_init(&num_threads_mutex, NULL);

  /* Kick off the real work now */
  init_html();
  run_variant(*variant);

  asprintf(&tmp,"Dirs: %d, Threads: %d",num_dirs,num_threads);
  write_output(tmp);
  asprintf(&tmp, "JPG: %d, BMP: %d, GIF: %d, PNG: %d",num_jpg, 
      num_bmp, num_gif, num_png);
  write_output(tmp);

  /* All done, clean up everything */
  pthread_cancel(log_thread);
  free(variant);
  finish_log();
  finish_output();
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
      inc_dirs();
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

/* One thread per directory */
void *do_v1(void *input_dir) {
  inc_threads();
  inc_dirs();
  DIR *dir;
  struct dirent *entry;
  const char *name = (char*)input_dir;
  char *tmp;
  asprintf(&tmp,"V1 START- Dir: %s",name);
  write_output(tmp);

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

  asprintf(&tmp,"V1 FINISH- Dir: %s",name);
  write_output(tmp);

  pthread_exit((void*) input_dir);
}

void *do_v2_help(void *v2struct) {
  inc_threads();
  v2struct_data_t *data = (v2struct_data_t *) v2struct;
  DIR *dir;
  struct dirent *entry;
  const char *name = data->dir;
  char *tmp;
  asprintf(&tmp,"V2 SUBTHR- Dir: %s",data->dir);
  write_output(tmp);

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
      inc_dirs();
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

/* 5 threads per directory - one for each of the four image types, one
 * for subdirectories */
void *do_v2(void *input_dir) {
  char *input_dir_s = (char *) input_dir;
  char *tmp;
  pthread_t dir_th, jpg_th, png_th, bmp_th, gif_th;
  v2struct_data_t dir_data, jpg_data, png_data, bmp_data, gif_data;
  asprintf(&tmp,"V2 START- Dir: %s",input_dir_s);
  write_output(tmp);

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
  
  asprintf(&tmp,"V2 FINISH- Dir: %s",input_dir_s);
  write_output(tmp);

  pthread_exit((void*) input_dir);
}

void *do_v3_img(void *file) {
  file_struct_t *tuple = build_file_struct(file);
  write_html(tuple);
  free(tuple);
  pthread_exit(NULL);
}

/* Level order traversal */
void *do_v3(void *input_dir) {
  inc_dirs();
  inc_threads();
  DIR *dir;
  struct dirent *entry;
  const char *name = (char*)input_dir;
  char *tmp;
  asprintf(&tmp,"V3 START- Dir: %s",name);
  write_output(tmp);

  char *subdir_strs[MAX_SUBDIRS];
  int cur_subdir_str = 0;

  pthread_t subdir_threads[MAX_SUBDIRS];
  pthread_t file_threads[MAX_FILES_PER_DIR];
  int cur_file_threads = 0;


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
      if(cur_subdir_str >= MAX_SUBDIRS) {
        asprintf(&tmp, "ERROR: Exceeded %d subdirectories in %s",MAX_SUBDIRS,name);
        write_output(tmp);
        break;
      }

      /* This is a directory */
      char *path; 
      asprintf(&path,"%s/%s",name,entry->d_name);
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
          continue;
      subdir_strs[cur_subdir_str++] = path;
    } else {
      /* This is a file, check if it's an image */
      char *tmp;
      asprintf(&tmp,"%s/%s",name,entry->d_name);
      if(IS_IMAGE(entry->d_name)) {
        /* It's an image, write its information to the HTML file */
        pthread_create(&file_threads[cur_file_threads++],NULL,do_v3_img,(void *)tmp);
      }
    }
  } while ((entry = readdir(dir)));
  closedir(dir);

  /* Join all image threads before working on subdirectories */
  int i;
  for(i = 0; i < cur_file_threads; i++) {
    void *status;
    pthread_join(file_threads[i],&status);
  }

  /* Start subdirectory threads */
  for(i = 0; i < cur_subdir_str; i++) {
    pthread_create(&subdir_threads[i],NULL,do_v3,(void *)subdir_strs[i]);
  }

  /* Join subdirectory threads */
  for(i = 0; i < cur_subdir_str; i++) {
    void *status;
    pthread_join(subdir_threads[i],&status);
  }

  asprintf(&tmp,"V3 FINISH- Dir: %s",name);
  write_output(tmp);

  pthread_exit((void*) input_dir);
}

/* Start timer for writing to log */
void *do_log(void *arg)
{
  arg = NULL;
  num_log_cycles = 1;
  while(1)
  {
    sleep(LOG_MS_DELAY/1000);
    int num_img = num_jpg + num_png + num_gif + num_bmp;
    char *tmp;
    asprintf(&tmp,"Time: %d ms #dir %d #files %d",num_log_cycles*LOG_MS_DELAY,
        num_dirs,num_img);
    write_log(tmp);
    ++num_log_cycles;
  }
  return 0;
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
  new_tuple->TimeOfModification = malloc(sizeof(struct timespec));
  *(new_tuple->TimeOfModification) = file_stat.st_mtim;
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

  /* Update appropriate image count variable */
  if(IS_JPG(file->FileName)) {
    inc_jpg();
  } else if(IS_PNG(file->FileName)) {
    inc_png();
  } else if(IS_GIF(file->FileName)) {
    inc_gif();
  } else if(IS_BMP(file->FileName)) {
    inc_bmp();
  }

  char *tmp;
  asprintf(&tmp,"Writing HTML for img: %s",file->FileName);
  write_output(tmp);

  /* Generate modified timestamp */
  struct tm t;
  if (localtime_r(&(file->TimeOfModification->tv_sec), &t) == NULL)
    return;

  char timestamp[256];
  const uint TIME_FMT = strlen("2012-12-31 12:59:59.123456789") + 1;

  if((strftime(timestamp, TIME_FMT, "%F %T", &t)) == 0)
    return;

  /* Write the data */
  pthread_mutex_lock(&html_mutex);
  fprintf(html_file,"<a href=../%s><img src=../%s width=100 height = 100></img>\
      </a><p align=\"left\">INode: %d, Name: %s, Type: %s, Size: %d bytes,\
      Last modified: %s, Thread ID: %ld</p>",file->FileName,
      file->FileName, file->FileId, file->FileName, file->FileType, file->Size,
      timestamp, file->ThreadId);  
  pthread_mutex_unlock(&html_mutex);
}

/* Complete and close the HTML file */
void finish_html() {
  fclose(html_file);
}

/* Complete and close the log file */
void finish_log() {
  /* Format the time nicely */
  char *tmp;
	char buffer[26];
	struct tm* tm_info;
	tm_info = localtime(&start_time);
	strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
  asprintf(&tmp,"\nProgram initiation: %s", buffer);
  write_log(tmp);

  /* Print out number of images */
  asprintf(&tmp,"\nNumber of jpg files = %d",num_jpg);
  write_log(tmp);
  asprintf(&tmp,"Number of bmp files = %d",num_bmp);
  write_log(tmp);
  asprintf(&tmp,"Number of png files = %d",num_png);
  write_log(tmp);
  asprintf(&tmp,"Number of gif files = %d",num_gif);
  write_log(tmp);

  asprintf(&tmp,"\nTotal number of valid image files = %d",num_gif + num_jpg
      + num_png + num_bmp);
  write_log(tmp);

  asprintf(&tmp,"\nTotal time of execution = %d",num_log_cycles);
  write_log(tmp);
  write_log("----------------------------------");
  asprintf(&tmp,"Number of threads created = %d",num_threads);
  write_log(tmp);

  fclose(log_file);
}
    
/* Complete and close the output file */
void finish_output() {
  write_output("Done");
  fclose(output_file);
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

/* Functions to update dir/image/thread counts... */
void inc_dirs() {
  pthread_mutex_lock(&num_dirs_mutex);
  ++num_dirs;
  pthread_mutex_unlock(&num_dirs_mutex);
}

void inc_dirs_by(int amount) {
  pthread_mutex_lock(&num_dirs_mutex);
  num_dirs += amount;
  pthread_mutex_unlock(&num_dirs_mutex);
}

void inc_jpg() {
  pthread_mutex_lock(&num_jpg_mutex);
  ++num_jpg;
  pthread_mutex_unlock(&num_jpg_mutex);
}

void inc_bmp() {
  pthread_mutex_lock(&num_bmp_mutex);
  ++num_bmp;
  pthread_mutex_unlock(&num_bmp_mutex);
}

void inc_png() {
  pthread_mutex_lock(&num_png_mutex);
  ++num_png;
  pthread_mutex_unlock(&num_png_mutex);
}

void inc_gif() {
  pthread_mutex_lock(&num_gif_mutex);
  ++num_gif;
  pthread_mutex_unlock(&num_gif_mutex);
}

void inc_threads() {
  pthread_mutex_lock(&num_threads_mutex);
  ++num_threads;
  pthread_mutex_unlock(&num_threads_mutex);
}
