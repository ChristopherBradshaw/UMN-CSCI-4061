/* Information
CSci 4061 Spring 2017 Assignment 2
Name1=Christopher Bradshaw
StudentID1=5300734
Commentary=This program converts images of various formats to jpgs and
  creates HTML pages out of the thumbnails which will automatically cycle
*/
#ifndef PARALLEL_CONVERT_H
#define PARALLEL_CONVERT_H

/* Constants / Macros */

#define LOG 0
#define DEBUG 1
#define DEBUG_MODE 0

#define USAGE "Usage: parallel_convert [convert_count] [output_dir] [input_dir]"
#define MAX_NUM_CHILDREN_PROCESSES 10
#define MAX_NUM_FILES 100
#define LOG_FILE_NAME "log.file"
#define JUNK_FILE_NAME "nonImage.txt"
#define OUTPUT_IMAGE_FORMAT ".jpg"
#define OUTPUT_IMAGE_DIMENSIONS "200x200"
#define HTML_CYCLE_INTERVAL 2
#define IS_GIF_PROCESS(pid) (pid % 3 == 0 && pid % 2 == 0)
#define IS_PNG_PROCESS(pid) (pid % 2 == 0 && pid % 3 != 0)
#define IS_BMP_PROCESS(pid) (pid % 3 == 0 && pid % 2 != 0)
#define IS_JUNK_PROCESS(pid) (pid % 3 != 0 && pid % 2 != 0)
#define IS_JUNK_TYPE(type) (strcmp(type,"png") && strcmp(type,"gif") && strcmp(type,"bmp"))

/* Variables */

/* Input directory name */
static char *input_dir;

/* Output directory name */
static char *output_dir;

/* Valid file names-- used for HTML generation (extensions not stored) */
static char *valid_files[MAX_NUM_FILES];
static int valid_files_size = 0;

/* Functions */

/* Selects a file from input_dir and processes it (or deletes it if it's junk) */
void handle_next_file(void);

/* Generates and writes the HTML file for this image */
void build_and_write_html_file(const char *base_name);

/* Returns next base file name to link to for the HTML page */
const char *next_html_file(const char *base_name);

/* Makes initial pass of the input directory- writes junk file names to a
 * junk log file and builds an array of valid files */
void analyze_input_directory(void);

/* Return the next file for this process to process */
const char *get_next_file(long pid);

/* Returns a new file name string without the extension (ex: a.jpg -> a) */
const char *get_file_name_no_ext(const char* f_name);

/* Return non-zero value if there are more
 * images to process. */
int more_files_to_process(void);

/* Return non-zero value if directory exists and is readable, false otherwise */
int read_dir(const char *dir_name);

/* Return the file extension (ex: a.jpg -> jpg) */
const char* get_file_type(const char *f_name);

/* Create the specified number of processes. 
 * Return non-zero value for invalid */
int create_children(int num_children);

/* Restarts wait if interrupted by a signal */
pid_t r_wait(int *stat_loc);

/* Writes msg to the log file */
void write_log(const char *msg, int level);

/* Removes the log file */
void clear_log(void);

/* Writes msg to the junk file */
void write_junk(const char *msg);

/* Removes the junk file */
void clear_junk(void);

/* Converts str to lower case */
void to_lower(char *str);

#endif
