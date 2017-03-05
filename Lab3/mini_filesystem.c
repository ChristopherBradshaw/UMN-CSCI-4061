/* Information
CSci 4061 Spring 2017 Assignment 3
Name1=Christopher Bradshaw
StudentID1=5300734
Commentary=This program simulates a virtual in-memory filesystem
*/

#include "mini_filesystem.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Filesystem call declarations */

int Search_Directory(char* filename);
int Add_to_Directory(char* filename, int inode_number);
Inode Inode_Read(int inode_number);
int Inode_Write(int inode_number, Inode input_inode);
int Block_Read(int block_number, int num_bytes, char* to_read);
int Block_Write(int block_number, int num_bytes, char* to_write);
Super_block Superblock_Read(void);
int Superblock_Write(Super_block input_superblock);

/* Debugging */
char *debug_str;
void debug(char *msg)
{
  if(DEBUG)
    fprintf(stderr,"DEBUG: %s\n",msg);
}


/* Filesystem interface definitions */

int Initialize_Filesystem(char* log_filename)
{
  Count = 0;
  Log_Filename = log_filename;
  Superblock.next_free_inode = 0;
  Superblock.next_free_block = 0;
  return 1;
}

/* Creates the specified file in the filesystem and 
 * returns the Inode number if it was created and a non-success
 * status (-1) if it already existed. */
int Create_File(char* filename, int UID, int GID, int filesize)
{
  if(Search_Directory(filename) != -1)
  {
    asprintf(&debug_str,"File %s already exists",filename);
    debug(debug_str);  
    return -1;
  }
  
  /* Construct the new Inode entry */
  Inode new_inode = 
  {
    .Inode_Number = Superblock_Read().next_free_inode,
    .User_Id = UID,
    .Group_Id = GID,
    .File_Size = filesize,
    .Start_Block = Superblock_Read().next_free_block,
    .End_Block = Superblock_Read().next_free_block 
      + (int) ceil((float)filesize / BLOCKSIZE),
    .Flag = 0,
  };
  
  /* Put the entry in the INode table */
  int inode_idx;
  if((inode_idx = Inode_Write(new_inode.Inode_Number,new_inode)) == -1)
  {
    asprintf(&debug_str,"Failed to insert Inode %d",new_inode.Inode_Number);
    debug("Failed to insert Inode");  
    return -1;
  }

  /* Create an entry in the directory for this new Inode */
  if(Add_to_Directory(filename, inode_idx) == -1)
  {
    asprintf(&debug_str,"Failed to create %s",filename);
    debug(debug_str);  
    return -1;
  } 

  /* Update the superblock */
  Super_block new_sb = 
  {
    .next_free_inode = inode_idx+1,
    .next_free_block = new_inode.End_Block + 1
  };
  Superblock_Write(new_sb);
  
  return inode_idx;
}

/* Attempts to open the specified file and 
 * returns the Inode number if it was found and a non-success
 * status (-1) otherwise. */
int Open_File(char* filename)
{
  int inode_idx;
  if((inode_idx = Search_Directory(filename)) == -1)
  {
    asprintf(&debug_str,"Could not open %s",filename);
    debug(debug_str);  
    return -1;
  }
  
  Inode inode = Inode_Read(inode_idx);
  inode.Flag = 1;  
  Inode_Write(inode_idx,inode);
  return inode_idx;
}

/* Attempts to read this Inode into to_read and returns the number
 * of bytes successfully read */
int Read_File(int inode_number, int offset, int count, char* to_read)
{
  // TODO assume inode_number is valid?
  Inode inode = Inode_Read(inode_number);
  if(offset+count >= inode.File_Size)
  {
    debug("Attempted read exceeds file size");  
    return 0;
  }

  /* Attempt to read the specified number of bytes */
  int total_read = 0;
  int current_block = inode.Start_Block;
  while(total_read <= count && current_block <= inode.End_Block)
  {
    int read = Block_Read(current_block,count-total_read,to_read+total_read);
    total_read += read;
    current_block++;
  }
  
  return total_read;
}

/* Attempts to write the contents of to_write into the 
 * data blocks for this Inode */
int Write_File(int inode_number, int offset, char* to_write)
{
  // TODO assume inode_number is valid?
  Inode inode = Inode_Read(inode_number);
  if(offset >= inode.File_Size)
  {
    debug("Attempted write exceeds file size");  
    return 0;
  }

  // TODO implement this...
  return (int)to_write;
}

/* Closes the specified file and returns 1 if it was sucessfully
 * closed and 0 if it was previously closed */
int Close_File(int inode_number)
{
  Inode inode = Inode_Read(inode_number);
  /* This file is open */
  if(inode.Flag == 1)
  {
    inode.Flag = 0;  
    Inode_Write(inode_number,inode);
    return 1;
  }

  /* This file was already closed */
  return 0;
}
