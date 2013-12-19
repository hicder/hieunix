#ifndef _FS_H
#define _FS_H

#include "types.h"
#include "lib.h"
#include "syscalls.h"

#define ENTRY_SIZE 64
#define BLOCK_SIZE 4096
#define FNAME_SIZE 32

#define RTC_FILE 0
#define DIR_FILE 1
#define REG_FILE 2
#define TMN_FILE 3
#define MAX_FILE_SIZE 62
#define MAX_BUF_SIZE MAX_FILE_SIZE*FNAME_SIZE


/* a 64B directory entry*/
typedef struct dentry
{
	uint8_t	fname[FNAME_SIZE];		// file name, zero padded, not necessarily including EOS or 0 byte
	uint32_t type;			// 0 rtc file, 1 directory, 2 regular file
	uint32_t inode;			// index node number for the file
	uint8_t reserved[24];	// 24 bytes reserved
} dentry_t;

/* a file entry in pcb, not used in cp2*/
typedef struct file
{
	uint32_t* optable;
	uint32_t* inode;
	uint32_t  file_pos;
	uint32_t  flags;
} file_t;





/*init function for file system */
extern void fs_init (uint32_t * adr);

/*file operetion functions */
extern int32_t file_open (fd_t* file_desc, const uint8_t* fname) ;
extern int32_t file_read(fd_t*  file_desc, uint8_t* buf, uint32_t nbytes);
extern int32_t file_write(fd_t*  file_desc, const uint8_t* buf, uint32_t nbytes);
extern int32_t file_close(fd_t*  file_desc);

/*directory operation functions */
extern int32_t dir_open (fd_t*  file_desc, const uint8_t* fname);
extern int32_t dir_read (fd_t*  file_desc, uint8_t* buf, uint32_t nbytes);
extern int32_t dir_write(fd_t*  file_desc, const uint8_t* buf, uint32_t nbytes);
extern int32_t dir_close(fd_t*  file_desc);


/*helper function to read dentry by name*/
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry);
/*helper function to read dentry by index*/
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
/*helper function to read data out from the file system*/
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

uint32_t get_file_length(unsigned int inode);


#endif

