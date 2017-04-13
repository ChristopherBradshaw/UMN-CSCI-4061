/* Information
CSci 4061 Spring 2017 Assignment 4
Name1=Christopher Bradshaw
StudentID1=5300734
Commentary=This program manages image files
*/

#ifndef IMAGE_MANAGER_H
#define IMAGE_MANAGER_H

#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define HTML_FILE "catalog.html"
#define LOG_FILE "catalog.log"
#define OUTPUT_FILE "output.log"
#define MAX_SUBDIRS 128
#define MAX_FILES_PER_DIR 256

typedef struct filestruct {
  int FileId; 
  char FileName[128]; 
  char *FileType;
  int Size;
  struct timespec *TimeOfModification;
  pthread_t ThreadId;
} file_struct_t;

typedef enum variant {
  V1,V2,V3
} variant_t;

typedef enum v2threadtype {
  DIRECTORY,JPG,PNG,BMP,GIF
} v2_thread_t;

typedef struct v2struct {
  char *dir;
  v2_thread_t type; 
} v2struct_data_t;

/* -------------- Data -------------- */

/* File pointer to log file */
static FILE *log_file;
pthread_mutex_t log_mutex;

/* File pointer to ouput file */
static FILE *output_file;
pthread_mutex_t output_mutex;

/* File pointer to HTML file */
static FILE *html_file;
pthread_mutex_t html_mutex;

/* Input directory containing image files/subdirectories */
static const char *input_dir;

/* Output directory where we place HTML file */
static const char *output_dir; 

/* Number of directories visited */
static int num_dirs;
pthread_mutex_t num_dirs_mutex;

/* Number of JPG images seen */
static int num_jpg;
pthread_mutex_t num_jpg_mutex;

/* Number of BMP images seen */
static int num_bmp;
pthread_mutex_t num_bmp_mutex;

/* Number of PNG images seen */
static int num_png;
pthread_mutex_t num_png_mutex;

/* Number of GIF images seen */
static int num_gif;
pthread_mutex_t num_gif_mutex;

/* Number of threads created */
static int num_threads;
pthread_mutex_t num_threads_mutex;

/* ----------- FUNCTIONS ------------ */

/* Traverses input directory (using a method specified by the variant).
 * Builds HTML file and handles logging. 
 * Return zero for success, non-zero for failure. */
int run_variant(variant_t variant);

/* Write to output file, must be thread safe */
void write_output(const char *str);

/* Write to log file, must be thread safe */
void write_log(const char *str);

/* Initialize the HTML file */
void init_html();

/* Write entry to HTML file */
void write_html(const file_struct_t *file);

/* Complete and close the HTML file */
void finish_html();

/* Functions to update dir/image/thread counts... */
void inc_dirs();
void inc_dirs_by(int amount);
void inc_jpg();
void inc_bmp();
void inc_png();
void inc_gif();
void inc_threads();

#endif
