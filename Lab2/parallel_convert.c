/* Information
CSci 4061 Spring 2017 Assignment 2
Name1=Christopher Bradshaw
StudentID1=5300734
Commentary=This program converts images of various formats to jpgs and
	creates HTML pages out of the thumbnails which will automatically cycle
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdio.h>
#include "parallel_convert.h"

int main(int argc, char **argv)
{
	long parent_pid = (long) getpid();
	long pid;

	/* Validate cmd line args */
	if(argc != 4)
	{
		fprintf(stderr,"%s\n",USAGE); 
		return 1;
	}

	/* Initialize some variables, make sure logs are clean */
	output_dir = argv[2];
	input_dir = argv[3];
	clear_junk();
	clear_log();
	write_log("Starting...\n",LOG);
	analyze_input_directory();

	/* Make sure input directory exists and is readable */
	if(!read_dir(input_dir)) 
	{
		write_log("Could not access input directory",LOG);	
		perror("Could not access input directory");
		return 1;
	}

	/* Make sure output directory exists and is accessable */
	if(!read_dir(output_dir)) 
	{
		write_log("Could not access output directory.. creating one",LOG);	
		mkdir(output_dir,0700);
	}

	/* Keep spawning children processes until all files have been managed */
	while(getpid() == parent_pid && more_files_to_process())
	{
		/* create_children returns non-zero status in the event of an error */
		if(create_children(atoi(argv[1])))
		{
			write_log("Could not create children processes",LOG);	
			return 1;
		}

		/* Process a file if we're a child process, 
		 * or wait for all children if we're the parent */
		if((long) getpid() != parent_pid)
			handle_next_file();			
		else
		{
			write_log("Waiting for all children to finish..",LOG);
			while(r_wait(NULL) > 0) ;
      write_log("Children have finished\n",LOG);
		}
	}

	if(getpid() == parent_pid)
		write_log("Done!",LOG);

	return 0;
}

/* Selects a file from input_dir and processes it (or deletes it if it's junk) */
void handle_next_file(void)
{
	long pid = (long) getpid();
	const char *f_name = get_next_file(pid);

	if(f_name != NULL)
	{
		if(IS_JUNK_PROCESS(pid))
		{
			/* We're going to be removing a junk file */
			const char *tmp;
			asprintf(&tmp, "Removing junk: %s", f_name);
			write_log(tmp,LOG);
			asprintf(&tmp,"%s/%s",input_dir,f_name);
			execlp("rm","rm",tmp,NULL);
		}
		else
		{
			/* We're going to be converting a file */
			const char *tmp;
			const char *src;
			const char *dst;
			const char *base_file = get_file_name_no_ext(f_name);
			asprintf(&src,"%s/%s",input_dir,f_name);
			asprintf(&dst,"%s/%s%s",output_dir,base_file,OUTPUT_IMAGE_FORMAT);

			/* In order to have the log statement printed AFTER conversion completes,
			 * we must fork execution again. The parent waits for child conversion to
			 * complete then prints/logs a message. The parent also handles
			 * HTML generation. */
			long child_pid = fork();
			if(child_pid != 0)
			{
				/* Parent builds HTML file and then waits for child conversion to complete 
				 * before logging the conversion */
				build_and_write_html_file(base_file);
				while(r_wait(NULL) > 0) ;
				asprintf(&tmp,"Converted file: %s -> %s of size by process %ld",\
						src,dst,pid);
				write_log(tmp,LOG);
			}
			else
			{
				/* Child converts the file */
				execlp("convert","convert","-resize",OUTPUT_IMAGE_DIMENSIONS,src,dst,NULL);
			}
		}
	}
}

/* Generates and writes the HTML file for this image */
void build_and_write_html_file(const char *base_name)
{
	char *output;
	char *dst;
	asprintf(&dst,"%s/%s.html",output_dir,base_name);

	FILE *fp = fopen(dst,"w");
	if(fp == NULL)
	{
		asprintf(&output,"Failed to write HTML file: %s", dst);
		write_log(output,LOG);
		perror("Failed to write HTML file");
	}
	else
	{
		/* This HTML file simply contains an image and automatically redirects
		 * to the next HTML file */
		asprintf(&output,"<html><head><title>%s</title></head><body>\
			<img src=%s%s /><meta http-equiv=\'refresh\' content=\"%d;URL=./%s.html\">\
			</body></html>\n",base_name,\
			base_name,OUTPUT_IMAGE_FORMAT,HTML_CYCLE_INTERVAL,next_html_file(base_name));

		fputs(output,fp);
		fclose(fp);
	}
}

/* Returns next base file name to link to for the HTML page */
const char *next_html_file(const char *base_name)
{
	int i;
	for(i = 0; i < valid_files_size; ++i)
	{
		if(strcmp(base_name,valid_files[i]) == 0)
		{
			/* Return the next index (treated as a circular array) */
			int next_idx = (i + 1 == valid_files_size ? 0 : i+1);
			return valid_files[next_idx];
		}
	}

	return NULL;
}

/* Makes initial pass of the input directory- writes junk file names to a
 * junk log file and builds an array of valid files */
void analyze_input_directory(void)
{
	DIR *input_dp;
	struct dirent *input_ep;
	input_dp = opendir(input_dir);

	if(input_dp != NULL)
	{
		while(input_ep = readdir(input_dp))
		{
			/* Ignore unimportant files */
			if(input_ep->d_type != DT_REG || !strcmp(input_ep->d_name,".DS_Store"))
				continue;

			char *type = (char *) get_file_type(input_ep->d_name);
			to_lower(type);

			if(IS_JUNK_TYPE(type))
			{
				/* Record this file name in the proper junk file */
				write_junk(input_ep->d_name);
			}
			else
			{
				/* If it's a valid file, store it in an array (this allows child processes
				 * to build HTML files that link to the "next" one) */
				char *base_name = (char *) get_file_name_no_ext(input_ep->d_name);
				valid_files[valid_files_size++] = base_name;
			}
		}
	}
	else
	{
		perror("Could not read input directory");
	}
}

/* Return the next file for this process to process */
const char *get_next_file(long pid)
{
	const char *next_file = NULL;
	const char *desired_file_type = NULL;

	/* What type of file should this PID be processing? */
	if(!IS_JUNK_PROCESS(pid))
	{
		if(IS_GIF_PROCESS(pid)) 
		{
			desired_file_type = "gif";
		}
		else if(IS_PNG_PROCESS(pid)) 
		{
			desired_file_type = "png";
		}
		else if(IS_BMP_PROCESS(pid)) 
		{
			desired_file_type = "bmp";
		}
	}

	DIR *input_dp;
	struct dirent *input_ep;
	input_dp = opendir(input_dir);

	if(input_dp != NULL)
	{
		while(input_ep = readdir(input_dp))
		{
			/* Ignore unimportant files */
			if(input_ep->d_type != DT_REG || !strcmp(input_ep->d_name,".DS_Store")) 
				continue;

			char *f_name = (char*) malloc(strlen(input_ep->d_name)+1);
			strcpy(f_name,input_ep->d_name);
			const char *f_name_no_ext = get_file_name_no_ext(f_name);
			char *type = (char *) get_file_type(f_name);
			to_lower(type);

			if(desired_file_type == NULL)
			{
				/* Junk file case, if this file isn't a png/gif/bmp, return it */
				if(IS_JUNK_TYPE(type))
				{
					next_file = f_name;
					break;
				}
			}
			else
			{
				/* We found a file matching this type in the input directory, 
				 * now check if it's already been converted (in the output directory) */
				if(strcmp(type,desired_file_type) == 0)
				{
					char *expected_f_name;
					asprintf(&expected_f_name,"%s/%s%s",
							output_dir,f_name_no_ext,OUTPUT_IMAGE_FORMAT);

					/* If this file hasn't been converted (not in output directory),
					 * we found the next file to choose. */
					if(access(expected_f_name,F_OK) == -1)
					{
						next_file = f_name;
						break;
					}
				}
			}

			free(f_name);
		}

		closedir(input_dp);
	}
	else
	{
		perror("Error while opening input directory");
	}

	return next_file;
}

/*	Returns a new file name string without the extension (ex: a.jpg -> a) */
const char *get_file_name_no_ext(const char* mystr)
{
    char *retstr;
    char *lastdot;
    if (mystr == NULL)
         return NULL;
    if ((retstr = malloc (strlen (mystr) + 1)) == NULL)
        return NULL;
    strcpy (retstr, mystr);
    lastdot = strrchr (retstr, '.');
    if (lastdot != NULL)
        *lastdot = '\0';
    return retstr;
}

/* Return non-zero value if there are more
 * images to process. */
int more_files_to_process(void)
{
	DIR *input_dp;
	struct dirent *input_ep;
	input_dp = opendir(input_dir);

	if(input_dp != NULL)
	{
		while(input_ep = readdir(input_dp))
		{
			/* Ignore unimportant files */
			if(input_ep->d_type != DT_REG || !strcmp(input_ep->d_name,".DS_Store")) 
				continue;

			char *tmp;
			asprintf(&tmp,"%s/%s%s",output_dir,
					get_file_name_no_ext(input_ep->d_name),OUTPUT_IMAGE_FORMAT);

			/* We found a file that hasn't been handled, need to keep doing work */
			if(access(tmp,F_OK) == -1)
			{
				asprintf(&tmp,"Found at least one more file to process:%s",tmp);
				write_log(tmp,DEBUG);
				return 1;
			}
		}
	}
	else
	{
		perror("Error while opening input directory");
	}

	return 0;
}

/* Return non-zero value if directory exists and is readable, false otherwise */
int read_dir(const char *dir_name)
{
	DIR* dir = opendir(dir_name);
	if (dir)
	{
		closedir(dir);
		return 1;
	}

	return 0;
}

/* Return the file extension (ex: a.jpg -> jpg) */
const char* get_file_type(const char *f_name)
{
	const char *tmp = strrchr(f_name,'.');

	/* Doesn't have an extension */
	if(tmp == NULL)
		return NULL;

	return tmp+1;	
}

/*
 * Attempts to create the specified number of children processes
 * Returns a non-zero value if the threads could not be created
 */
int create_children(int num_children)
{
	pid_t childpid, parent_pid = (long)getpid();
	int i;

	if(num_children <= 0 || num_children > MAX_NUM_CHILDREN_PROCESSES)
	{
		fprintf(stderr,"Error: number of children processes must be in the range 1-%d\n",MAX_NUM_CHILDREN_PROCESSES);
		return -1;	
	}

	write_log("Creating children processes...",LOG);

	/* Spawn the correct number of children. Child processes do not
	 * get any children of their own. */
	for(i = 0; i < num_children; ++i)
		if((childpid = fork()) <= 0)
			break;

	if((long)getpid() != parent_pid)
	{
		/* This is a newly created child */
		char *msg;
		asprintf(&msg, "Created child process [ID:%ld, Parent ID:%ld]",
				(long)getpid(),(long)getppid());
		write_log(msg,DEBUG);
	}

	return 0;
}

/* Restarts wait if interrupted by a signal (Robbins pg. 72) */
pid_t r_wait(int *stat_loc)
{
	int retval;
	while(((retval = wait(stat_loc)) == -1) && (errno == EINTR)) ;
	return retval;
}

/* Writes msg to the log file */
void write_log(const char *msg, int level)
{
	if(level == DEBUG && !DEBUG_MODE)
		return;

  printf("%s\n",msg);
	char *f_name;
	asprintf(&f_name,"%s/%s",output_dir,LOG_FILE_NAME);

	FILE *f = fopen(f_name, "ab+");
	if (f)
	{
		fprintf(f,"> %s\n", msg);
		fclose(f);
	}
	else
	{
		perror("Could not open log file");
	}
}

/* Removes the log file */
void clear_log()
{
	char *f_name;
	asprintf(&f_name,"%s/%s",output_dir,LOG_FILE_NAME);

	if(access(f_name, F_OK) != -1)
		remove(f_name);
}

/* Writes msg to the junk file */
void write_junk(const char *msg)
{
	char *f_name;
	asprintf(&f_name,"%s/%s",output_dir,JUNK_FILE_NAME);

	FILE *f = fopen(f_name, "ab+");
	if (f)
	{
		fprintf(f,"%s\n", msg);
		fclose(f);
	}	
	else
	{
		perror("Could not open junk file");
	}
}

/* Removes the junk file */
void clear_junk()
{
	char *f_name;
	asprintf(&f_name,"%s/%s",output_dir,JUNK_FILE_NAME);

	if(access(JUNK_FILE_NAME, F_OK) != -1)
		remove(JUNK_FILE_NAME);
}

/* Converts str to lower case */
void to_lower(char *str)
{
	for ( ; *str; ++str) *str= tolower(*str);
}
