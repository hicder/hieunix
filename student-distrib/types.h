/* types.h - Defines to use the familiar explicitly-sized types in this
 * OS (uint32_t, int8_t, etc.).  This is necessary because we don't want
 * to include <stdint.h> when building this OS
 * vim:ts=4 noexpandtab
 */

#ifndef _TYPES_H
#define _TYPES_H

#define NULL 0
#define ERROR -1
#define MAX_BUFFER_SIZE 128
#ifndef ASM

/* Types defined here just like in <stdint.h> */
typedef int int32_t;
typedef unsigned int uint32_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef char int8_t;
typedef unsigned char uint8_t;


struct fops_table;	//resolving circular typedef

/* File Descriptor */

typedef struct file_desc {

	struct fops_table * fops_p; // pointer to file operations table for the process
	uint32_t* inode_p;	   // node index pointer
	uint32_t file_pos;	   // current read position
	uint32_t flags;		   // 1 = in use, 0 = not in use

} fd_t;

/* File Operations Table */
typedef uint32_t (*read_func)(fd_t* file_desc, uint8_t* buf, uint32_t nbytes);
typedef uint32_t (*write_func)(fd_t* file_desc,const uint8_t* buf, uint32_t nbytes);
typedef uint32_t (*close_func)(fd_t* file_desc);

typedef struct fops_table{

	uint32_t* open; 						// points to appropriate open system call
	close_func close;
	read_func read;			// points to appropriate read system call
	write_func write;		// points to appropriate write system call

} fops_table_t;
typedef struct registers{
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint32_t esi;
	uint32_t edi;
	uint32_t ebp;
	uint32_t eax;
	uint32_t flags2;
	uint32_t eip;
	uint32_t cs;
	uint32_t flags;
	uint32_t esp;
	uint32_t ds;
}reg_t;

/* Process Control Block */

typedef struct pcb{

	fd_t file_desc[8]; // file descriptor array for process	
	struct pcb * parent;			  // parent process of current process
	//fops_table_t fops_tables[8]; // file operations table for process
	uint32_t pt_idx;	// the index of the which page table is being used. when halt, have to clear the page
	uint32_t pcb_idx;
  // for halt, register values of parent processes
    uint32_t old_esp;        //esp of parent process
    uint32_t old_eip;        //eip of parent process
    uint32_t old_eflags;
    uint32_t old_ebp;
    uint32_t old_ss;
    uint32_t old_cs;
    uint32_t old_tssESP;
    // return value for any process executed within this program
    int32_t return_val;
	uint32_t argument_buffer_size; 
	uint32_t terminal_number;
	uint32_t tssESP;
	uint32_t eip;
	uint32_t esp;
	uint32_t ebp;
	uint32_t flags;
	char argument_buffer[MAX_BUFFER_SIZE];
} pcb_t;




#endif /* ASM */

#endif /* _TYPES_H */
