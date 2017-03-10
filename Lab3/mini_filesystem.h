/* Information
CSci 4061 Spring 2017 Assignment 3
Name1=Christopher Bradshaw
StudentID1=5300734
Commentary=This program simulates a virtual in-memory filesystem
*/

#ifndef MINI_FILESYSTEM_H
#define MINI_FILESYSTEM_H

#define MAXFILES 128
#define MAXBLOCKS 8192
#define BLOCKSIZE 512

typedef struct superblock
{
    int next_free_inode;
    int next_free_block;

}Super_block;

typedef struct inode
{
    int Inode_Number;
    int User_Id;
    int Group_Id;
    int File_Size;
    int Start_Block;
    int End_Block;
    int Flag;
}Inode;

typedef struct directory
{
    char Filename[21];
    int Inode_Number;
}Directory;

/* Declare Filesystem structures */
Super_block Superblock;
Directory Directory_Structure[MAXFILES];
Inode Inode_List[MAXFILES];
char* Disk_Blocks[MAXBLOCKS];

int inode_list_size;
int directory_size;

/* Declare variable for Count and Log Filename */
int Count;
char* Log_Filename;

/* Filesystem Interface Declaration
   See the assignment for more details */

int Initialize_Filesystem(char* log_filename);
int Create_File(char* filename, int UID, int GID, int filesize);
int Open_File(char* filename);
int Read_File(int inode_number, int offset, int count, char* to_read);
int Write_File(int inode_number, int offset, char* to_write);
int Close_File(int inode_number);

/* Utility functions */
typedef enum LOG_LEVEL {NONE,INFO,DEBUG} log_level_t;
log_level_t global_logging;

void SET_LOG_LEVEL(log_level_t level);
void LOG(char*, enum LOG_LEVEL);
void print_memory(void);

#endif
