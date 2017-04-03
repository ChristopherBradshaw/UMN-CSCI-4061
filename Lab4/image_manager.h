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

typedef struct filestruct {
  int FileId; 
  char FileName[128]; 
  char FileType[8];
  int Size;
  time_t TimeOfModification;
  pthread_t ThreadId;
} file_struct_t;


#endif
