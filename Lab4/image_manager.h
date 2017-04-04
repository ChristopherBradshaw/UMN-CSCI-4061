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

#define LOG_FILE "catalog.log"
#define OUTPUT_FILE "output.log"

typedef struct filestruct {
  int FileId; 
  char FileName[128]; 
  char FileType[8];
  int Size;
  time_t TimeOfModification;
  pthread_t ThreadId;
} file_struct_t;

typedef enum variant {
  V1,V2,V3
} variant_t;

/* -------------- Data -------------- */

/* File pointer to log file */
FILE *log_file;

/* File pointer to ouput file */
FILE *output_file;

/* Input directory containing image files/subdirectories */
const char *input_dir;

/* Output directory where we place HTML file */
const char *output_dir; 

/* ----------- FUNCTIONS ------------ */

/* Traverses input directory (using a method specified by the variant).
 * Builds HTML file and handles logging. 
 * Return zero for success, non-zero for failure. */
int run_variant(variant_t variant);

/* Write to output file, must be thread safe */
void write_output(const char *str);

/* Write to log file, must be thread safe */
void write_log(const char *str);

#endif
