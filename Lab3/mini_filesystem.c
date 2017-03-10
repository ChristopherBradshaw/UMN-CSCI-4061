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
#include <string.h>

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
char *LOGSTR;

void SET_LOG_LEVEL(log_level_t level)
{
  global_logging = level;
}

void LOG(char *msg, log_level_t logging)
{
  if(global_logging >= INFO && logging == INFO)
    fprintf(stderr,"INFO: %s\n",msg);
  else if(global_logging >= DEBUG && logging == DEBUG)
    fprintf(stderr,"DEBUG: %s\n",msg);
}

void print_memory(void)
{
  int i;
  for(i = 0; i < MAXBLOCKS; i++)
  {
    printf("%d:|%s|\n",i,Disk_Blocks[i]);
  }
}


/* Filesystem interface definitions */

int Initialize_Filesystem(char* LOG_filename)
{
  Count = 0;
  Log_Filename = LOG_filename;
  Superblock.next_free_inode = 0;
  Superblock.next_free_block = 0;
  directory_size = 0;
  inode_list_size = 0;
  /* Initialize blocks */
  int i;
  for(i = 0; i < MAXBLOCKS; i++)
  {
    Disk_Blocks[i] = calloc(BLOCKSIZE, sizeof(char));
  }

  return 1;
}

/* Creates the specified file in the filesystem and 
 * returns the Inode number if it was created and a non-success
 * status (-1) if it already existed. */
int Create_File(char* filename, int UID, int GID, int filesize)
{
  if(Search_Directory(filename) != -1)
  {
    asprintf(&LOGSTR,"File %s already exists",filename);
    LOG(LOGSTR, DEBUG);  
    return -1;
  }
  
  /* Construct the new Inode entry */
  Inode new_inode = 
  {
    .Inode_Number = Superblock_Read().next_free_inode,
    .User_Id = UID,
    .Group_Id = GID,
    .File_Size = 0,
    .Start_Block = Superblock_Read().next_free_block,
    .End_Block = Superblock_Read().next_free_block 
      + (int) ceil((float)filesize / BLOCKSIZE) - 1,
    .Flag = 0,
  };
  
  asprintf(&LOGSTR,"Created INode: (#%d,Start:%d,End:%d)", new_inode.Inode_Number, new_inode.Start_Block,
      new_inode.End_Block);
  LOG(LOGSTR,INFO);

  /* Put the entry in the INode table */
  int inode_idx;
  if((inode_idx = Inode_Write(new_inode.Inode_Number,new_inode)) == -1)
  {
    asprintf(&LOGSTR,"Failed to insert Inode %d",new_inode.Inode_Number);
    LOG("Failed to insert Inode", DEBUG);
    return -1;
  }

  /* Create an entry in the directory for this new Inode */
  if(Add_to_Directory(filename, inode_idx) == -1)
  {
    asprintf(&LOGSTR,"Failed to create %s",filename);
    LOG(LOGSTR, DEBUG);  
    return -1;
  } 

  /* Update the superblock */
  Super_block new_sb = 
  {
    .next_free_inode = inode_idx+1,
    .next_free_block = new_inode.End_Block + 1
  };
  Superblock_Write(new_sb);
  
  asprintf(&LOGSTR,"Created file (%s,#%d)",filename, inode_idx);
  LOG(LOGSTR, INFO);
  return inode_idx;
}

/* Attempts to open the specified file and 
 * returns the Inode number if it was found and a non-success
 * status (-1) otherwise. */
int Open_File(char* filename)
{
  asprintf(&LOGSTR,"Attempting to open: %s", filename);
  LOG(LOGSTR, DEBUG);

  int inode_idx;
  if((inode_idx = Search_Directory(filename)) == -1)
  {
    asprintf(&LOGSTR,"Could not open %s",filename);
    LOG(LOGSTR, DEBUG);  
    return -1;
  }

  asprintf(&LOGSTR,"Found inode: %s:%d", filename, inode_idx);
  LOG(LOGSTR, DEBUG);

  Inode inode = Inode_Read(inode_idx);
  inode.Flag = 1;  
  Inode_Write(inode_idx,inode);

  asprintf(&LOGSTR,"Opened file (%s,#%d)",filename, inode_idx);
  LOG(LOGSTR, INFO);
  return inode_idx;
}

/* Attempts to read this Inode into to_read and returns the number
 * of bytes successfully read */
int Read_File(int inode_number, int offset, int count, char* to_read)
{
  // TODO assume inode_number is valid?
  Inode inode = Inode_Read(inode_number);
  if(offset+count > inode.File_Size)
  {
    asprintf(&LOGSTR,"Failed to read %d bytes (INode %d, filesize %d)",
        offset+count,inode.Inode_Number,inode.File_Size);
    LOG(LOGSTR,INFO);
    return -1;
  }

  /* Attempt to read the specified number of bytes */
  int total_read = 0;
  int current_block = inode.Start_Block;
  while(total_read <= count && current_block <= inode.End_Block)
  {
    int n_read = Block_Read(current_block,count-total_read,to_read+total_read);
    total_read += n_read;
    current_block++;
  }
  
  asprintf(&LOGSTR,"Read %d bytes from file (?,#%d)",total_read, inode.Inode_Number);
  LOG(LOGSTR, INFO);
  return total_read;
}

/* Attempts to write the contents of to_write into the 
 * data blocks for this Inode */
int Write_File(int inode_number, int offset, char* to_write)
{
  // TODO assume inode_number is valid?
  Inode inode = Inode_Read(inode_number);
  if(offset > inode.File_Size)
  {
    LOG("Attempted write exceeds file size", DEBUG);  
    return -1;
  }

  // TODO assuming we ignore offset?
  int total_write = 0;
  int current_block = inode.Start_Block;
  while(*(to_write+total_write) && current_block <= inode.End_Block)
  {
    int n_write = Block_Write(current_block,BLOCKSIZE,to_write+total_write);
    total_write += n_write;
    current_block++;
  }

  inode.File_Size += total_write;
  Inode_Write(inode.Inode_Number,inode);

  // TODO not updating superblock since blocks are allocated based on
  // provided filesize

  asprintf(&LOGSTR,"Wrote %d bytes to file (?,#%d) New size: %d",total_write, 
      inode.Inode_Number, inode.File_Size);
  LOG(LOGSTR, INFO);
  return total_write;
}

/* Closes the specified file and returns 0 if it was sucessfully
 * closed and -1 if it was previously closed */
int Close_File(int inode_number)
{
  Inode inode = Inode_Read(inode_number);
  /* This file is open */
  if(inode.Flag == 1)
  {
    inode.Flag = 0;  
    Inode_Write(inode_number,inode);
    asprintf(&LOGSTR,"Closed file (?,#%d)", inode.Inode_Number);
    LOG(LOGSTR, INFO);
    return 0;
  }

  /* This file was already closed */
  return -1;
}

/* Attempt to find the specifid file in the directory structure,
 * return the inode if success, -1 otherwise. */
int Search_Directory(char* filename)
{
  int i;
  for(i = 0; i < directory_size; i++)
  {
    if(strcmp(Directory_Structure[i].Filename,filename) == 0)
      return Directory_Structure[i].Inode_Number;
  }

  return -1;
}

/* Attempt to add the specified file to the directory structure,
 * return 1 if success, -1 otherwise. */
int Add_to_Directory(char* filename, int inode_number)
{
  if(directory_size >= MAXFILES)
  {
    /* Directory is full */
    LOG("Could not add to directory, too many files", DEBUG);
    return -1;
  }

  /* Add the new entry to the directory */
  Directory dir_entry;
  strcpy(dir_entry.Filename,filename);
  dir_entry.Inode_Number = inode_number;
  Directory_Structure[directory_size++] = dir_entry;
  return 1;
}

/* Attempt to retrieve the specified Inode.
 * Return the struct if success, or a dummy struct (Inode_Number == -1)
 * otherwise. */
Inode Inode_Read(int inode_number)
{
  if(inode_number >= inode_list_size)
  {
    /* Invalid inode index */
    LOG("Invalid inode number", DEBUG);
    Inode bad_node = {.Inode_Number = -1};
    return bad_node;
  }

  asprintf(&LOGSTR,"Reading INode: %d",inode_number);
  LOG(LOGSTR,DEBUG);
  return Inode_List[inode_number];
}

/* Attempts to write the specified inode to the specified index,
 * return 1 if success, -1 otherwise. */
int Inode_Write(int inode_number, Inode input_inode)
{
  if(inode_number >= MAXFILES)
  {
    /* Invalid inode index */
    LOG("Invalid inode number", INFO);
    return -1;
  }

  asprintf(&LOGSTR, "Wrote INode: %d %d",inode_number, input_inode.File_Size);
  LOG(LOGSTR,DEBUG);
  Inode_List[inode_number] = input_inode;
  return inode_list_size++;
}

/* Read the specified number of bytes from the blocks and write
 * it to the given string. Return the number of bytes read. */
int Block_Read(int block_number, int num_bytes, char* to_read)
{
  if(block_number >= MAXBLOCKS)
  {
    LOG("Invalid block number", INFO);
    return -1;
  }

  char *block = Disk_Blocks[block_number];
  int num_read = 0;

  /* Read the specified number of bytes or until 
   * we reach the end (max or null character) */
  while(num_read < num_bytes && num_read < BLOCKSIZE)
  {
    if(block[num_read] == 0)
      break;

    to_read[num_read] = block[num_read];
    num_read++;
  }

  return num_read;
}

/* Write the specified number of bytes to the blocks.
 * Return the number of bytes written. */
int Block_Write(int block_number, int num_bytes, char* to_write)
{
  if(block_number >= MAXBLOCKS)
  {
    LOG("Invalid block number", INFO);
    return -1;
  }

  char *block = Disk_Blocks[block_number];
  int num_write = 0;

  /* Write the specified number of bytes or until we reach the end */
  while(num_write < num_bytes && num_write < BLOCKSIZE && to_write[num_write])
  {
    block[num_write] = to_write[num_write];
    num_write++;
  }

  return num_write;
}

/* Get the superblock */
Super_block Superblock_Read(void)
{
  return Superblock;
}

/* Set the superblock */
int Superblock_Write(Super_block input_superblock)
{
  Superblock = input_superblock;
  return 1;
}
