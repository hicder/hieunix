#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include "lib.h"
#include "types.h"
#include "fs.h"
#include "rtc.h"
#include "terminal.h"
#include "x86_desc.h"
#include "page.h"

#define EXEC_ADDR 0x08048000
#define USER_STACK 0X08400000
#define KERNEL_STACK_BOT 0x800000
#define PCB_OFFSET 0x2000
#define PCB_MASK  0xffffe000
#define FIRST_PROG  (KERNEL_STACK_BOT-PCB_OFFSET)
#define USER_PROG_ADDR 0x8000000
/* All calls return >= 0 on success or -1 on failure. */

/*  
 * Note that the system call for halt will have to make sure that only
 * the low byte of EBX (the status argument) is returned to the calling
 * task.  Negative returns from execute indicate that the desired program
 * could not be found.
 */ 
extern int32_t halt (uint8_t status);
extern int32_t execute (const uint8_t* command);
extern int32_t read (int32_t fd, void* buf, int32_t nbytes);
extern int32_t write (int32_t fd, const void* buf, int32_t nbytes);
extern int32_t open (const uint8_t* filename);
extern int32_t close (int32_t fd);
extern int32_t getargs (uint8_t* buf, int32_t nbytes);
extern int32_t vidmap (uint8_t** screen_start);
extern int32_t set_handler (int32_t signum, void* handler);
extern int32_t sigreturn (void);


// fops struct
fops_table_t terminal_fops;
fops_table_t rtc_fops;
fops_table_t file_fops;
fops_table_t dir_fops;

enum signums {
	DIV_ZERO = 0,
	SEGFAULT,
	INTERRUPT,
	ALARM,
	USER1,
	NUM_SIGNALS
};

// Helper function
pcb_t* get_pcb() ;
void set_up_fops();
#endif
